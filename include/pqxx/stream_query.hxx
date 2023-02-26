/* Definition of the pqxx::stream_query class.
 *
 * pqxx::stream_query enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_query instead.
 *
 * Copyright (c) 2000-2023, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STREAM_QUERY
#define PQXX_H_STREAM_QUERY

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <cassert>
#include <functional>
#include <variant>

#include "pqxx/connection.hxx"
#include "pqxx/except.hxx"
#include "pqxx/internal/concat.hxx"
#include "pqxx/internal/encoding_group.hxx"
#include "pqxx/internal/encodings.hxx"
#include "pqxx/internal/gates/connection-stream_from.hxx"
#include "pqxx/internal/stream_iterator.hxx"
#include "pqxx/separated_list.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/transaction_focus.hxx"
#include "pqxx/util.hxx"


namespace pqxx
{
class transaction_base;


// XXX: Move to pqxx::internal.  Call only through transaction::stream().
/// Stream query results from the database.
/** For larger data sets, retrieving data this way is likely to be faster than
 * executing a query and then iterating and converting the rows fields.  You
 * will also be able to start processing before all of the data has come in.
 * (For smaller result sets though, it's likely to be slower.)
 *
 * This class is similar to @ref stream_from, but it's more strongly typed.
 * You specify the column fields while instantiating the stream_query template.
 *
 * Not all kinds of query will work in a stream.  But straightforward `SELECT`
 * and `UPDATE ... RETURNING` queries should work.  The class uses PostgreSQL's
 * `COPY` command, so see the documentation for that command to get the full
 * details.
 *
 * There are other downsides.  If the stream encounters an error, it may leave
 * the entire connection in an unusable state, so you'll have to give the
 * whole thing up.  Finally, opening a stream puts the connection in a special
 * state, so you won't be able to do many other things with the connection or
 * the transaction while the stream is open.
 *
 * Usually you'll want the `stream` convenience wrapper in
 * @ref transaction_base, * so you don't need to deal with this class directly.
 *
 * @warning While a stream is active, you cannot execute queries, open a
 * pipeline, etc. on the same transaction.  A transaction can have at most one
 * object of a type derived from @ref pqxx::transaction_focus active on it at a
 * time.
 */
template<typename... TYPE>
class PQXX_LIBEXPORT stream_query : transaction_focus
{
public:
  using raw_line =
    std::pair<std::unique_ptr<char, std::function<void(char *)>>, std::size_t>;

  inline stream_query(transaction_base &tx, std::string_view query);

  ~stream_query() noexcept
  {
    try
    {
      close();
    }
    catch (std::exception const &e)
    {
      reg_pending_error(e.what());
    }
  }

  bool done() const & noexcept { return m_finished; }

  /// Finish this stream.  Call this before continuing to use the connection.
  /** Consumes all remaining lines, and closes the stream.
   *
   * This may take a while if you're abandoning the stream before it's done, so
   * skip it in error scenarios where you're not planning to use the connection
   * again afterwards.
   */
  void complete()
  {
    if (done())
      return;
    try
    {
      // Flush any remaining lines - libpq will automatically close the stream
      // when it hits the end.
      while (not done()) get_raw_line();
    }
    catch (broken_connection const &)
    {
      close();
      throw;
    }
    catch (std::exception const &e)
    {
      reg_pending_error(e.what());
    }
    close();
  }

  /// Read one row into a tuple.
  /** Converts the row's fields into the fields making up the tuple.
   *
   * For a column which can contain nulls, be sure to give the corresponding
   * tuple field a type which can be null.  For example, to read a field as
   * `int` when it may contain nulls, read it as `std::optional<int>`.
   * Using `std::shared_ptr` or `std::unique_ptr` will also work.
   */
  void receive_row(std::tuple<TYPE...> &t) &
  {
    // XXX: Do we actually need to check for this here?
    if (done())
      PQXX_UNLIKELY return;
    static constexpr auto tup_size{sizeof...(TYPE)};
    parse_line();
    if (done())
      PQXX_UNLIKELY return;

    extract_fields(t, std::make_index_sequence<tup_size>{});
    return;
  }

  /// Doing this with a `std::variant` is going to be horrifically borked.
  template<typename... Vs>
  stream_query &operator>>(std::variant<Vs...> &) = delete;

  inline auto begin() &;
  inline auto end() const &;

private:
  static pqxx::internal::char_finder_func *
  get_finder(transaction_base const &tx);

  /// Read a raw line of text from the COPY command.
  inline auto get_raw_line() &;

  template<std::size_t... indexes>
  void
  extract_fields(std::tuple<TYPE...> &t, std::index_sequence<indexes...>)
  const &
  {
    (extract_value<indexes>(t), ...);
  }

  template<std::size_t index>
  void extract_value(std::tuple<TYPE...> &t) const &
  {
    using field_type = strip_t<decltype(std::get<index>(t))>;
    using nullity = nullness<field_type>;
    static_assert(index < sizeof...(TYPE));
    if constexpr (nullity::always_null)
    {
      if (std::data(m_fields[index]) != nullptr)
        throw conversion_error{"Streaming non-null value into null field."};
    }
    else if (std::data(m_fields[index]) == nullptr)
    {
      if constexpr (nullity::has_null)
        std::get<index>(t) = nullity::null();
      else
        internal::throw_null_conversion(type_name<field_type>);
    }
    else
    {
      // Don't ever try to convert a non-null value to nullptr_t!
      std::get<index>(t) = from_string<field_type>(m_fields[index]);
    }
  }

  /// Read a line of COPY data, write `m_row` and `m_fields`.
  void parse_line() &
  {
    assert(not done());

    // Which field are we currently parsing?
    std::size_t field_idx{0};

    auto const [line, line_size] = get_raw_line();
    if (done()) return;

    if (line_size >= (std::numeric_limits<decltype(line_size)>::max() / 2))
      throw range_error{"Stream produced a ridiculously long line."};

    // Make room for unescaping the line.  It's a pessimistic size.
    // Unusually, we're storing terminating zeroes *inside* the string.
    // This is the only place where we modify m_row.  MAKE SURE THE BUFFER DOES
    // NOT GET RESIZED while we're working, because we're working with views
    // into its buffer.
    m_row.resize(line_size + 1);

    char const *line_begin{line.get()};
    std::string_view const line_view{line_begin, line_size};

    // Output iterator for unescaped text.
    char *write{m_row.data()};

    // The pointer cannot be null at this point.  But we initialise field_begin
    // with this value, and carry it around the loop, and it can later become
    // null.  Static analysis in clang-tidy then likes to assume a case where
    // field_begin is null, and deduces from this that "write" must have been
    // null -- and so it marks "*write" as a null pointer dereference.
    //
    // This assertion tells clang-tidy just what it needs in order to deduce
    // that *write never dereferences a null pointer.
    assert(write != nullptr);

    // Beginning of current field in m_row, or nullptr for null fields.
    char const *field_begin{write};

    std::size_t offset{0};
    while (offset < line_size)
    {
      auto const stop_char{m_char_finder(line_view, offset)};
      // Copy the text we have so far.  It's got no special characters in it.
      std::memcpy(write, &line_begin[offset], stop_char - offset);
      write += (stop_char - offset);
      if (stop_char >= line_size)
        break;
      offset = stop_char;

      char const special{line_begin[stop_char]};
      ++offset;
      if (special == '\t')
      {
        // Field separator.  End the field.
        if (field_begin == nullptr)
        {
          m_fields[field_idx] = zview{};
          ++field_idx;
        }
        else
        {
	  m_fields[field_idx] = zview{field_begin, write - field_begin};
          *write++ = '\0';
	  ++field_idx;
        }
        // Set up for the next field.
        field_begin = write;
      }
      else
      {
        // Escape sequence.
        assert(special == '\\');
        if ((offset) >= line_size)
          throw failure{"Row ends in backslash"};

        // The database will only escape ASCII characters, so no need to use
        // the glyph scanner.
        char const escaped{line_begin[offset++]};
        if (escaped == 'N')
        {
          // Null value.
          if (write != field_begin)
            throw failure{"Null sequence found in nonempty field"};
          field_begin = nullptr;
          // (If there's any characters _after_ the null we'll just crash.)
        }
        *write++ = pqxx::internal::unescape_char(escaped);
      }
    }

    // End the last field here.
    if (field_begin == nullptr)
    {
      m_fields[field_idx] = zview{};
    }
    else
    {
      m_fields[field_idx] = zview{field_begin, write - field_begin};
      *write++ = '\0';
    }

    // DO NOT shrink m_row to fit.  We're carrying string_views pointing into
    // the buffer.  (Also, how useful would shrinking really be?)
  }

  std::string_view s_class_name{"stream_query"};

// XXX: Hoist choice of scanner up to higher level in call graph.
  pqxx::internal::char_finder_func *m_char_finder;

  /// Current row's fields' text, combined into one reusable string.
  std::string m_row;

  /// The current row's fields.
  std::array<zview, sizeof...(TYPE)> m_fields;

// XXX: Could this be implicit in our other state somewhere?
  bool m_finished = false;

  void close()
  {
    if (not done())
    {
      PQXX_UNLIKELY
      m_finished = true;
      unregister_me();
    }
  }
};
} // namespace pqxx
#endif
