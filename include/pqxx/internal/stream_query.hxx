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


/// The `end()` iterator for a `stream_query`.
class stream_query_end_iterator {};


// C++20: Can we use generators, and maybe get speedup from HALO?
/// Stream query results from the database.
/** For larger data sets, retrieving data this way is likely to be faster than
 * executing a query and then iterating and converting the rows' fields.  You
 * will also be able to start processing before all of the data has come in.
 * (For smaller result sets though, a stream is likely to be a bit slower.)
 *
 * A `stream_query` stream is strongly typed.  You specify the columns' types
 * while instantiating the stream_query template.
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
template<typename... TYPE> class stream_query : transaction_focus
{
public:

  /// Execute `query` on `tx`, stream results.
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

  /// Has this stream reached the end of its data?
  bool done() const &noexcept { return m_finished; }

  inline auto begin() &;
  inline auto end() const & { return stream_query_end_iterator{}; }

  /// Parse and convert the latest line of data we received.
  std::tuple<TYPE...> parse_line() &
  {
    assert(not done());

    // This function uses m_row as a buffer, across calls.  The only reason for
    // it to carry over across calls is to avoid reallocation.

// TODO: We could probably parse fields as we go, without this array.
    // The last-read row's fields, as views into m_rows.
    std::array<zview, sizeof...(TYPE)> fields;

    // Which field are we currently parsing?
    std::size_t field_idx{0};

    auto const line_size{m_line_size};

    // Make room for unescaping the line.  It's a pessimistic size.
    // Unusually, we're storing terminating zeroes *inside* the string.
    // This is the only place where we modify m_row.  MAKE SURE THE BUFFER DOES
    // NOT GET RESIZED while we're working, because we're working with views
    // into its buffer.
    m_row.resize(line_size + 1);

    char const *line_begin{m_line.get()};
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
          fields[field_idx] = zview{};
        }
        else
        {
          fields[field_idx] = zview{field_begin, write - field_begin};
          *write++ = '\0';
        }
        // Set up for the next field.
        field_begin = write;
        ++field_idx;
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
      fields[field_idx] = zview{};
    }
    else
    {
      fields[field_idx] = zview{field_begin, write - field_begin};
      *write++ = '\0';
    }
    ++field_idx;

    if (field_idx != sizeof...(TYPE))
      throw usage_error{pqxx::internal::concat(
        "Trying to stream query into ", sizeof...(TYPE),
        " column(s), "
        "but received ",
        field_idx, ".")};

    // DO NOT shrink m_row to fit.  We're carrying views pointing into the
    // buffer.  (Also, how useful would shrinking really be?)

    return extract_fields(std::make_index_sequence<sizeof...(TYPE)>{}, fields);
  }

  /// Read a line from the server, into `m_line` and `m_line_size`.
  auto read_line() &;

private:
  /// Look up a char_finder_func.
  /** This is the only encoding-dependent code in the class.  All we need to
   * store after that is this function pointer.
   */
  static inline pqxx::internal::char_finder_func *
  get_finder(transaction_base const &tx);

  /// Extract values for the fields, and return them as a tuple.
  template<std::size_t... indexes>
  std::tuple<TYPE...>
  extract_fields(
    std::index_sequence<indexes...>,
    std::array<zview, sizeof...(TYPE)> const &fields
  ) const &
  {
    return std::tuple<TYPE...>{extract_value<indexes>(fields)...};
  }

  /// Extract type `#n` from our parameter pack, `TYPE...`.
  template<std::size_t n>
  using extract_t = decltype(std::get<n>(std::declval<std::tuple<TYPE...>>()));

  template<std::size_t index> auto extract_value(
    std::array<zview, sizeof...(TYPE)> const &fields) const &
  {
    using field_type = strip_t<extract_t<index>>;
    using nullity = nullness<field_type>;
    static_assert(index < sizeof...(TYPE));
    if constexpr (nullity::always_null)
    {
      if (std::data(fields[index]) != nullptr)
        throw conversion_error{"Streaming non-null value into null field."};
    }
    else if (std::data(fields[index]) == nullptr)
    {
      if constexpr (nullity::has_null)
        return nullity::null();
      else
        internal::throw_null_conversion(type_name<field_type>);
    }
    else
    {
      // Don't ever try to convert a non-null value to nullptr_t!
      return from_string<field_type>(fields[index]);
    }
  }

  pqxx::internal::char_finder_func *m_char_finder;

  /// Current row's fields' text, combined into one reusable string.
  std::string m_row;

  /// Buffer (allocated by libpq) holding the last line we read.
  std::unique_ptr<char, std::function<void(char *)>> m_line;

  /// Size of the last line we read.
  std::size_t m_line_size{0u};

  /// Has our iteration finished?
  bool m_finished = false;

  void close() noexcept
  {
    if (not done())
    {
      m_finished = true;
      unregister_me();
    }
  }
};
} // namespace pqxx
#endif
