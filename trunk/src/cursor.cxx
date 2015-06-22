/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of libpqxx STL-style cursor classes.
 *   These classes wrap SQL cursors in STL-like interfaces
 *
 * Copyright (c) 2004-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cstdlib>
#include <cstring>

#include "pqxx/cursor"
#include "pqxx/result"
#include "pqxx/strconv"
#include "pqxx/transaction"

#include "pqxx/internal/gates/connection-sql_cursor.hxx"
#include "pqxx/internal/gates/icursor_iterator-icursorstream.hxx"
#include "pqxx/internal/gates/icursorstream-icursor_iterator.hxx"
#include "pqxx/internal/gates/result-sql_cursor.hxx"

using namespace pqxx;
using namespace pqxx::internal;


namespace
{
/// Is this character a "useless trailing character" in a query?
/** A character is "useless" at the end of a query if it is either whitespace or
 * a semicolon.
 */
inline bool useless_trail(char c)
{
  return isspace(c) || c==';';
}

}


pqxx::internal::sql_cursor::sql_cursor(transaction_base &t,
	const std::string &query,
	const std::string &cname,
	cursor_base::accesspolicy ap,
	cursor_base::updatepolicy up,
	cursor_base::ownershippolicy op,
	bool hold) :
  cursor_base(t.conn(), cname),
  m_home(t.conn()),
  m_empty_result(),
  m_adopted(false),
  m_at_end(-1),
  m_pos(0),
  m_endpos(-1)
{
  if (&t.conn() != &m_home) throw internal_error("Cursor in wrong connection");
  std::stringstream cq, qn;

  /* Strip trailing semicolons (and whitespace, as side effect) off query.  The
   * whitespace is stripped because it might otherwise mask a semicolon.  After
   * this, the remaining useful query will be the sequence defined by
   * query.begin() and last, i.e. last may be equal to query.end() or point to
   * the first useless trailing character.
   */
  std::string::const_iterator last = query.end();
  // TODO: May break on multibyte encodings!
  for (--last; last!=query.begin() && useless_trail(*last); --last) ;
  if (last==query.begin() && useless_trail(*last))
    throw argument_error("Cursor created on empty query");
  ++last;

  cq << "DECLARE \"" << name() << "\" ";

  m_home.activate();

  if (ap == cursor_base::forward_only) cq << "NO ";
  cq << "SCROLL ";

  cq << "CURSOR ";

  if (hold) cq << "WITH HOLD ";

  cq << "FOR " << std::string(query.begin(),last) << ' ';

  if (up != cursor_base::update) cq << "FOR READ ONLY ";
  else cq << "FOR UPDATE ";

  qn << "[DECLARE " << name() << ']';
  t.exec(cq, qn.str());

  // Now that we're here in the starting position, keep a copy of an empty
  // result.  That may come in handy later, because we may not be able to
  // construct an empty result with all the right metadata due to the weird
  // meaning of "FETCH 0."
  init_empty_result(t);

  // If we're creating a WITH HOLD cursor, noone is going to destroy it until
  // after this transaction.  That means the connection cannot be deactivated
  // without losing the cursor.
  if (hold)
    gate::connection_sql_cursor(t.conn()).add_reactivation_avoidance_count(1);

  m_ownership = op;
}


pqxx::internal::sql_cursor::sql_cursor(transaction_base &t,
	const std::string &cname,
	cursor_base::ownershippolicy op) :
  cursor_base(t.conn(), cname, false),
  m_home(t.conn()),
  m_empty_result(),
  m_adopted(true),
  m_at_end(0),
  m_pos(-1),
  m_endpos(-1)
{
  // If we take responsibility for destroying the cursor, that's one less reason
  // not to allow the connection to be deactivated and reactivated.
  // TODO: Go over lifetime/reactivation rules again to be sure they work
  if (op==cursor_base::owned)
    gate::connection_sql_cursor(t.conn()).add_reactivation_avoidance_count(-1);
  m_adopted = true;
  m_ownership = op;
}


void pqxx::internal::sql_cursor::close() PQXX_NOEXCEPT
{
  if (m_ownership==cursor_base::owned)
  {
    try
    {
      gate::connection_sql_cursor(m_home).Exec(
	("CLOSE \"" + name() + "\"").c_str(),
	0);
    }
    catch (const std::exception &)
    {
    }

    if (m_adopted)
      gate::connection_sql_cursor(m_home).add_reactivation_avoidance_count(-1);

    m_ownership = cursor_base::loose;
  }
}


void pqxx::internal::sql_cursor::init_empty_result(transaction_base &t)
{
  if (pos() != 0) throw internal_error("init_empty_result() from bad pos()");
  m_empty_result = t.exec("FETCH 0 IN \"" + name() + '"');
}


/// Compute actual displacement based on requested and reported displacements
internal::sql_cursor::difference_type
pqxx::internal::sql_cursor::adjust(difference_type hoped,
	difference_type actual)
{
  if (actual < 0) throw internal_error("Negative rows in cursor movement");
  if (hoped == 0) return 0;
  const int direction = ((hoped < 0) ? -1 : 1);
  bool hit_end = false;
  if (actual != labs(hoped))
  {
    if (actual > labs(hoped))
      throw internal_error("Cursor displacement larger than requested");

    // If we see fewer rows than requested, then we've hit an end (on either
    // side) of the result set.  Wether we make an extra step to a one-past-end
    // position or whether we're already there depends on where we were
    // previously: if our last move was in the same direction and also fell
    // short, we're already at a one-past-end row.
    if (m_at_end != direction) ++actual;

    // If we hit the beginning, make sure our position calculation ends up
    // at zero (even if we didn't previously know where we were!), and if we
    // hit the other end, register the fact that we now know where the end
    // of the result set is.
    if (direction > 0) hit_end = true;
    else if (m_pos == -1) m_pos = actual;
    else if (m_pos != actual)
      throw internal_error("Moved back to beginning, but wrong position: "
        "hoped=" + to_string(hoped) + ", "
        "actual=" + to_string(actual) + ", "
        "m_pos=" + to_string(m_pos) + ", "
        "direction=" + to_string(direction));

    m_at_end = direction;
  }
  else
  {
    m_at_end = 0;
  }

  if (m_pos >= 0) m_pos += direction*actual;
  if (hit_end)
  {
    if (m_endpos >= 0 && m_pos != m_endpos)
      throw internal_error("Inconsistent cursor end positions");
    m_endpos = m_pos;
  }
  return direction*actual;
}


result pqxx::internal::sql_cursor::fetch(difference_type rows,
	difference_type &displacement)
{
  if (!rows)
  {
    displacement = 0;
    return m_empty_result;
  }
  const std::string query =
      "FETCH " + stridestring(rows) + " IN \"" + name() + "\"";
  const result r(gate::connection_sql_cursor(m_home).Exec(query.c_str(), 0));
  displacement = adjust(rows, difference_type(r.size()));
  return r;
}


cursor_base::difference_type pqxx::internal::sql_cursor::move(
	difference_type rows,
	difference_type &displacement)
{
  if (!rows)
  {
    displacement = 0;
    return 0;
  }

  const std::string query =
      "MOVE " + stridestring(rows) + " IN \"" + name() + "\"";
  const result r(gate::connection_sql_cursor(m_home).Exec(query.c_str(), 0));
  difference_type d = difference_type(r.affected_rows());
  displacement = adjust(rows, d);
  return d;
}


std::string pqxx::internal::sql_cursor::stridestring(difference_type n)
{
  /* Some special-casing for ALL and BACKWARD ALL here.  We used to use numeric
   * "infinities" for difference_type for this (the highest and lowest possible
   * values for "long"), but for PostgreSQL 8.0 at least, the backend appears to
   * expect a 32-bit number and fails to parse large 64-bit numbers.
   * We could change the typedef to match this behaviour, but that would break
   * if/when Postgres is changed to accept 64-bit displacements.
   */
  static const std::string All("ALL"), BackAll("BACKWARD ALL");
  if (n >= cursor_base::all()) return All;
  else if (n <= cursor_base::backward_all()) return BackAll;
  return to_string(n);
}


pqxx::cursor_base::cursor_base(connection_base &context,
	const std::string &Name,
	bool embellish_name) :
  m_name(embellish_name ? context.adorn_name(Name) : Name)
{
}


result::size_type pqxx::internal::obtain_stateless_cursor_size(sql_cursor &cur)
{
  if (cur.endpos() == -1) cur.move(cursor_base::all());
  return result::size_type(cur.endpos() - 1);
}


result pqxx::internal::stateless_cursor_retrieve(
	sql_cursor &cur,
	result::difference_type size,
	result::difference_type begin_pos,
	result::difference_type end_pos)
{
  if (begin_pos < 0 || begin_pos > size)
    throw range_error("Starting position out of range");

  if (end_pos < -1) end_pos = -1;
  else if (end_pos > size) end_pos = size;

  if (begin_pos == end_pos) return cur.empty_result();

  const int direction = ((begin_pos < end_pos) ? 1 : -1);
  cur.move((begin_pos-direction) - (cur.pos()-1));
  return cur.fetch(end_pos - begin_pos);
}


pqxx::icursorstream::icursorstream(
    transaction_base &context,
    const std::string &query,
    const std::string &basename,
    difference_type sstride) :
  m_cur(context,
	query,
	basename,
	cursor_base::forward_only,
	cursor_base::read_only,
	cursor_base::owned,
	false),
  m_stride(sstride),
  m_realpos(0),
  m_reqpos(0),
  m_iterators(0),
  m_done(false)
{
  set_stride(sstride);
}


pqxx::icursorstream::icursorstream(
    transaction_base &context,
    const field &cname,
    difference_type sstride,
    cursor_base::ownershippolicy op) :
  m_cur(context, cname.c_str(), op),
  m_stride(sstride),
  m_realpos(0),
  m_reqpos(0),
  m_iterators(0),
  m_done(false)
{
  set_stride(sstride);
}


void pqxx::icursorstream::set_stride(difference_type n)
{
  if (n < 1)
    throw argument_error("Attempt to set cursor stride to " + to_string(n));
  m_stride = n;
}

result pqxx::icursorstream::fetchblock()
{
  const result r(m_cur.fetch(m_stride));
  m_realpos += r.size();
  if (r.empty()) m_done = true;
  return r;
}


icursorstream &pqxx::icursorstream::ignore(std::streamsize n)
{
  difference_type offset = m_cur.move(difference_type(n));
  m_realpos += offset;
  if (offset < n) m_done = true;
  return *this;
}


icursorstream::size_type pqxx::icursorstream::forward(size_type n)
{
  m_reqpos += difference_type(n) * m_stride;
  return icursorstream::size_type(m_reqpos);
}


void pqxx::icursorstream::insert_iterator(icursor_iterator *i) PQXX_NOEXCEPT
{
  gate::icursor_iterator_icursorstream(*i).set_next(m_iterators);
  if (m_iterators)
    gate::icursor_iterator_icursorstream(*m_iterators).set_prev(i);
  m_iterators = i;
}


void pqxx::icursorstream::remove_iterator(icursor_iterator *i)
	const PQXX_NOEXCEPT
{
  gate::icursor_iterator_icursorstream igate(*i);
  if (i == m_iterators)
  {
    m_iterators = igate.get_next();
    if (m_iterators)
      gate::icursor_iterator_icursorstream(*m_iterators).set_prev(0);
  }
  else
  {
    icursor_iterator *prev = igate.get_prev(), *next = igate.get_next();
    gate::icursor_iterator_icursorstream(*prev).set_next(next);
    if (next) gate::icursor_iterator_icursorstream(*next).set_prev(prev);
  }
  igate.set_prev(0);
  igate.set_next(0);
}


void pqxx::icursorstream::service_iterators(difference_type topos)
{
  if (topos < m_realpos) return;

  typedef std::multimap<difference_type,icursor_iterator*> todolist;
  todolist todo;
  for (icursor_iterator *i = m_iterators, *next; i; i = next)
  {
    gate::icursor_iterator_icursorstream gate(*i);
    const icursor_iterator::difference_type ipos = gate.pos();
    if (ipos >= m_realpos && ipos <= topos)
      todo.insert(todolist::value_type(ipos, i));
    next = gate.get_next();
  }
  const todolist::const_iterator todo_end(todo.end());
  for (todolist::const_iterator i = todo.begin(); i != todo_end; )
  {
    const difference_type readpos = i->first;
    if (readpos > m_realpos) ignore(readpos - m_realpos);
    const result r = fetchblock();
    for ( ; i != todo_end && i->first == readpos; ++i)
      gate::icursor_iterator_icursorstream(*i->second).fill(r);
  }
}


pqxx::icursor_iterator::icursor_iterator() PQXX_NOEXCEPT :
  m_stream(0),
  m_here(),
  m_pos(0),
  m_prev(0),
  m_next(0)
{
}

pqxx::icursor_iterator::icursor_iterator(istream_type &s) PQXX_NOEXCEPT :
  m_stream(&s),
  m_here(),
  m_pos(difference_type(gate::icursorstream_icursor_iterator(s).forward(0))),
  m_prev(0),
  m_next(0)
{
  gate::icursorstream_icursor_iterator(*m_stream).insert_iterator(this);
}

pqxx::icursor_iterator::icursor_iterator(const icursor_iterator &rhs)
	PQXX_NOEXCEPT :
  m_stream(rhs.m_stream),
  m_here(rhs.m_here),
  m_pos(rhs.m_pos),
  m_prev(0),
  m_next(0)
{
  if (m_stream) 
    gate::icursorstream_icursor_iterator(*m_stream).insert_iterator(this);
}


pqxx::icursor_iterator::~icursor_iterator() PQXX_NOEXCEPT
{
  if (m_stream)
    gate::icursorstream_icursor_iterator(*m_stream).remove_iterator(this);
}


icursor_iterator pqxx::icursor_iterator::operator++(int)
{
  icursor_iterator old(*this);
  m_pos = difference_type(
	gate::icursorstream_icursor_iterator(*m_stream).forward());
  m_here.clear();
  return old;
}


icursor_iterator &pqxx::icursor_iterator::operator++()
{
  m_pos = difference_type(
	gate::icursorstream_icursor_iterator(*m_stream).forward());
  m_here.clear();
  return *this;
}


icursor_iterator &pqxx::icursor_iterator::operator+=(difference_type n)
{
  if (n <= 0)
  {
    if (!n) return *this;
    throw argument_error("Advancing icursor_iterator by negative offset");
  }
  m_pos = difference_type(
	gate::icursorstream_icursor_iterator(*m_stream).forward(
		icursorstream::size_type(n)));
  m_here.clear();
  return *this;
}


icursor_iterator &
pqxx::icursor_iterator::operator=(const icursor_iterator &rhs) PQXX_NOEXCEPT
{
  if (rhs.m_stream == m_stream)
  {
    m_here = rhs.m_here;
    m_pos = rhs.m_pos;
  }
  else
  {
    if (m_stream)
      gate::icursorstream_icursor_iterator(*m_stream).remove_iterator(this);
    m_here = rhs.m_here;
    m_pos = rhs.m_pos;
    m_stream = rhs.m_stream;
    if (m_stream)
      gate::icursorstream_icursor_iterator(*m_stream).insert_iterator(this);
  }
  return *this;
}


bool pqxx::icursor_iterator::operator==(const icursor_iterator &rhs) const
{
  if (m_stream == rhs.m_stream) return pos() == rhs.pos();
  if (m_stream && rhs.m_stream) return false;
  refresh();
  rhs.refresh();
  return m_here.empty() && rhs.m_here.empty();
}


bool pqxx::icursor_iterator::operator<(const icursor_iterator &rhs) const
{
  if (m_stream == rhs.m_stream) return pos() < rhs.pos();
  refresh();
  rhs.refresh();
  return !m_here.empty();
}


void pqxx::icursor_iterator::refresh() const
{
  if (m_stream)
    gate::icursorstream_icursor_iterator(*m_stream).service_iterators(pos());
}


void pqxx::icursor_iterator::fill(const result &r)
{
  m_here = r;
}
