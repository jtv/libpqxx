/** Implementation of libpqxx STL-style cursor classes.
 *
 * These classes wrap SQL cursors in STL-like interfaces.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cctype>
#include <iterator>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/cursor.hxx"
#include "pqxx/internal/encodings.hxx"
#include "pqxx/internal/gates/connection-sql_cursor.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;

namespace
{
/// Is this character a "useless trailing character" in a query?
/** A character is "useless" at the end of a query if it is either whitespace
 * or a semicolon.
 */
PQXX_PURE inline constexpr bool useless_trail(char c) noexcept
{
  return (c == ' ') or (c == '\t') or (c == '\n') or (c == '\r') or (c == ';');
}


// LCOV_EXCL_START
/// Find end of nonempty query, stripping off any trailing semicolon.
/** When executing a normal query, a trailing semicolon is meaningless but
 * won't hurt.  That's why we can't rule out that some code may include one.
 *
 * But for cursor queries, a trailing semicolon is a problem.  The query gets
 * embedded in a larger statement, which a semicolon would break into two.
 * We'll have to remove it if present.
 *
 * A trailing semicolon may not actually be at the end.  It could be masked by
 * subsequent whitespace.  If there's also a comment though, that's the
 * caller's own lookout.  We can't guard against every possible mistake, and
 * text processing is actually remarkably sensitive to mistakes in a
 * multi-encoding world.
 *
 * If there is a trailing semicolon, this function returns its offset.  If
 * there are more than one, it returns the offset of the first one.  If there
 * is no trailing semicolon, it returns the length of the query string.
 *
 * The query must be nonempty.
 */
constexpr std::string::size_type
find_query_end(std::string_view query, pqxx::encoding_group enc, pqxx::sl loc)
{
  auto const size{std::size(query)};

  // Marker for the end of the last "useful" character in the query.
  std::size_t end{};
  if (enc == pqxx::encoding_group::ascii_safe)
  {
    // This is an encoding where we can just scan backwards from the end.
    for (end = size; end > 0 and useless_trail(query[end - 1]); --end);
  }
  else
  {
    // Complex encoding.  We only know how to iterate forwards, so start from
    // the beginning.
    end = 0;

    // Look for ASCII whitespace & semicolons.  Really we're looking for
    // anything that's _not_ one of those.
    auto const finder{
      pqxx::internal::get_char_finder<' ', '\t', '\n', '\r', ';'>(enc, loc)};

    for (std::size_t here{0}, next{}; here < size; here = next + 1)
    {
      next = finder(query, here, loc);
      if ((next - here) > 0)
        // Found something that's not whitespace or semicolon.  Move the "end"
        // marker to the location right after it.
        end = next;
    }
  }
  return end;
}


#if !defined(NDEBUG)
/// Convenience shorthand: Invoke @ref find_query_end for compile-time testing.
consteval auto check_query_end(
  pqxx::encoding_group enc, std::string_view query,
  pqxx::sl loc = pqxx::sl::current())
{
  return find_query_end(query, enc, loc);
}


static_assert(check_query_end(pqxx::encoding_group::ascii_safe, "") == 0);
static_assert(check_query_end(pqxx::encoding_group::ascii_safe, ";  ") == 0);
static_assert(check_query_end(pqxx::encoding_group::ascii_safe, "ABC") == 3);
static_assert(check_query_end(pqxx::encoding_group::ascii_safe, "X Y") == 3);
static_assert(check_query_end(pqxx::encoding_group::ascii_safe, "n  ") == 1);
static_assert(check_query_end(pqxx::encoding_group::ascii_safe, " n ") == 2);
static_assert(check_query_end(pqxx::encoding_group::ascii_safe, "? ; ") == 1);
static_assert(
  check_query_end(pqxx::encoding_group::ascii_safe, " ( ; ) ") == 6);

static_assert(check_query_end(pqxx::encoding_group::two_tier, "") == 0);
static_assert(check_query_end(pqxx::encoding_group::two_tier, ";  ") == 0);
static_assert(check_query_end(pqxx::encoding_group::two_tier, "ABC") == 3);
static_assert(check_query_end(pqxx::encoding_group::two_tier, "X Y") == 3);
static_assert(check_query_end(pqxx::encoding_group::two_tier, "n  ") == 1);
static_assert(check_query_end(pqxx::encoding_group::two_tier, " n ") == 2);
static_assert(check_query_end(pqxx::encoding_group::two_tier, "? ; ") == 1);
static_assert(check_query_end(pqxx::encoding_group::two_tier, " ( ; ) ") == 6);
#endif // NDEBUG
// LCOV_EXCL_STOP
} // namespace


pqxx::internal::sql_cursor::sql_cursor(
  transaction_base &t, std::string_view query, std::string_view cname,
  cursor_base::access_policy ap, cursor_base::update_policy up,
  cursor_base::ownership_policy op, bool hold, sl loc) :
        cursor_base{t.conn(), cname},
        m_home{t.conn()},
        m_at_end{-1},
        m_pos{0},
        m_created_loc{loc}
{
  if (&t.conn() != &m_home)
    throw internal_error{"Using cursor in the wrong connection.", loc};

  if (std::empty(query))
    throw usage_error{"Cursor has empty query.", loc};
  auto const enc{t.conn().get_encoding_group(loc)};
  auto const qend{find_query_end(query, enc, loc)};
  if (qend == 0)
    throw usage_error{"Cursor has effectively empty query.", loc};
  query.remove_suffix(std::size(query) - qend);

  std::string const cq{std::format(
    "DECLARE {} {} SCROLL CURSOR {} FOR {} {}", t.quote_name(name()),
    ((ap == cursor_base::forward_only) ? "NO" : ""), (hold ? "WITH HOLD" : ""),
    query, ((up == cursor_base::update) ? "FOR UPDATE "sv : "FOR READ ONLY"))};

  t.exec(cq, loc);

  // Now that we're here in the starting position, keep a copy of an empty
  // result.  That may come in handy later, because we may not be able to
  // construct an empty result with all the right metadata due to the weird
  // meaning of "FETCH 0."
  init_empty_result(t, loc);

  m_ownership = op;
}


pqxx::internal::sql_cursor::sql_cursor(
  transaction_base &t, std::string_view cname,
  cursor_base::ownership_policy op, sl loc) :
        cursor_base{t.conn(), cname, false},
        m_home{t.conn()},
        m_ownership{op},
        m_at_end{0},
        m_pos{-1},
        m_created_loc{loc}
{}


pqxx::internal::sql_cursor::~sql_cursor() noexcept
{
  try
  {
    close(m_created_loc);
  }
  catch (std::exception const &e)
  {
    m_home.process_notice(
      std::format("Error closing cursor {}: {}", name(), e.what()));
    // Not much more we can do!
  }
}


void pqxx::internal::sql_cursor::close(sl loc)
{
  if (m_ownership == cursor_base::owned)
  {
    gate::connection_sql_cursor{m_home}.exec(
      std::format("CLOSE {}", m_home.quote_name(name())).c_str(), loc);
    m_ownership = cursor_base::loose;
  }
}


void pqxx::internal::sql_cursor::init_empty_result(transaction_base &t, sl loc)
{
  if (pos() != 0)
    throw internal_error{"init_empty_result() from bad pos().", loc};
  m_empty_result =
    t.exec(std::format("FETCH 0 IN {}", m_home.quote_name(name())), loc);
}


/// Compute actual displacement based on requested and reported displacements.
pqxx::internal::sql_cursor::difference_type pqxx::internal::sql_cursor::adjust(
  difference_type hoped, difference_type actual)
{
  if (actual < 0)
    throw internal_error{"Negative rows in cursor movement."};
  if (hoped == 0)
    return 0;
  int const direction{((hoped < 0) ? -1 : 1)};
  bool hit_end{false};
  if (actual != labs(hoped))
  {
    if (actual > labs(hoped))
      throw internal_error{"Cursor displacement larger than requested."};

    // If we see fewer rows than requested, then we've hit an end (on either
    // side) of the result set.  Wether we make an extra step to a one-past-end
    // position or whether we're already there depends on where we were
    // previously: if our last move was in the same direction and also fell
    // short, we're already at a one-past-end row.
    if (m_at_end != direction)
      ++actual;

    // If we hit the beginning, make sure our position calculation ends up
    // at zero (even if we didn't previously know where we were!), and if we
    // hit the other end, register the fact that we now know where the end
    // of the result set is.
    if (direction > 0)
      hit_end = true;
    else if (m_pos == -1)
      m_pos = actual;
    else if (m_pos != actual)
      throw internal_error{std::format(
        "Moved back to beginning, but wrong position: hoped={}, "
        "actual={}, m_pos={}, direction={}.",
        hoped, actual, m_pos, direction)};

    m_at_end = direction;
  }
  else
  {
    m_at_end = 0;
  }

  if (m_pos >= 0)
    m_pos += direction * actual;
  if (hit_end)
  {
    if (m_endpos >= 0 and m_pos != m_endpos)
      throw internal_error{"Inconsistent cursor end positions."};
    m_endpos = m_pos;
  }
  return direction * actual;
}


pqxx::result pqxx::internal::sql_cursor::fetch(
  difference_type rows, difference_type &displacement, sl loc)
{
  if (rows == 0)
  {
    displacement = 0;
    return m_empty_result;
  }
  auto const query{std::format(
    "FETCH {} IN {}", stridestring(rows), m_home.quote_name(name()))};
  auto r{gate::connection_sql_cursor{m_home}.exec(query.c_str(), loc)};
  displacement = adjust(rows, difference_type(std::size(r)));
  return r;
}


pqxx::cursor_base::difference_type pqxx::internal::sql_cursor::move(
  difference_type rows, difference_type &displacement, sl loc)
{
  if (rows == 0)
  {
    displacement = 0;
    return 0;
  }

  auto const query{std::format(
    "MOVE {} IN {}", stridestring(rows), m_home.quote_name(name()))};
  auto const r{gate::connection_sql_cursor{m_home}.exec(query.c_str(), loc)};
  auto d{static_cast<difference_type>(r.affected_rows())};
  displacement = adjust(rows, d);
  return d;
}


std::string pqxx::internal::sql_cursor::stridestring(difference_type n)
{
  /* Some special-casing for ALL and BACKWARD ALL here.  We used to use numeric
   * "infinities" for difference_type for this (the highest and lowest possible
   * values for "long"), but for PostgreSQL 8.0 at least, the backend appears
   * to expect a 32-bit number and fails to parse large 64-bit numbers. We
   * could change the alias to match this behaviour, but that would break
   * if/when Postgres is changed to accept 64-bit displacements.
   */
  static std::string const All{"ALL"}, BackAll{"BACKWARD ALL"};
  if (n >= cursor_base::all())
    return All;
  else if (n <= cursor_base::backward_all())
    return BackAll;
  return to_string(n);
}
