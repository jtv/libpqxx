/* Definition of the pqxx::stream_to class.
 *
 * pqxx::stream_to enables optimized batch updates to a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_to.hxx instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STREAM_TO
#define PQXX_H_STREAM_TO

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/separated_list.hxx"
#include "pqxx/transaction_base.hxx"


namespace pqxx
{
/// Efficiently write data directly to a database table.
/** If you wish to insert rows of data into a table, you can compose INSERT
 * statements and execute them.  But it's slow and tedious, and you need to
 * worry about quoting and escaping the data.
 *
 * If you're just inserting a single row, it probably won't matter much.  You
 * can use prepared or parameterised statements to take care of the escaping
 * for you.  But if you're inserting large numbers of rows you will want
 * something better.
 *
 * Inserting rows one by one using INSERT statements involves a lot of
 * pointless overhead, especially when you are working with a remote database
 * server over the network.  You may end up sending each row over the network
 * as a separate query, and waiting for a reply.  Do it "in bulk" using
 * @c stream_to, and you may find that it goes many times faster.  Sometimes
 * you gain orders of magnitude in speed.
 *
 * Here's how it works: you create a @c stream_to stream to start writing to
 * your table.  You will probably want to specify the columns.  Then, you
 * feed your data into the stream one row at a time.  And finally, you call the
 * stream's @c complete() to tell it to finalise the operation, wait for
 * completion, and check for errors.
 *
 * So how do you feed a row of data into the stream?  There's several ways, but
 * the preferred one is to call its @c write_values.  Pass the field values as
 * arguments.  Doesn't matter what type they are, as long as libpqxx knows how
 * to convert them to PostgreSQL's text format: @c int, @c std::string or
 * @c std:string_view, @c float and @c double, @c bool...  lots of basic types
 * are supported.  If some of the values are null, feel free to use
 * @c std::optional, @c std::shared_ptr, or @c std::unique_ptr.
 *
 * The arguments' types don't even have to match the fields' SQL types.  If you
 * want to insert an @c int into a @c DECIMAL column, that's your choice -- it
 * will produce a @c DECIMAL value which happens to be integral.  Insert a
 * @c float into a @c VARCHAR column?  That's fine, you'll get a string whose
 * contents happen to read like a number.  And so on.  You can even insert
 * different types of value in the same column on different rows.  If you have
 * a code path where a particular field is always null, just insert @c nullptr.
 *
 * There is another way to insert rows: the @c << ("shift-left") operator.
 * It's not as fast and it doesn't support variable arguments: each row must be
 * either a @c std::tuple or something iterable, such as a @c std::vector, or
 * anything else with a @c begin and @c end.
 */
class PQXX_LIBEXPORT stream_to : internal::transactionfocus
{
public:
  /// Create a stream, without specifying columns.
  /** Fields will be inserted in whatever order the columns have in the
   * database.
   *
   * You'll probably want to specify the columns, so that the mapping between
   * your data fields and the table is explicit in your code, and not hidden
   * in an "implicit contract" between your code and your schema.
   */
  stream_to(transaction_base &, std::string_view table_name);

  /// Create a stream, specifying column names as a container of strings.
  template<typename Columns>
  stream_to(
    transaction_base &, std::string_view table_name, Columns const &columns);

  /// Create a stream, specifying column names as a sequence of strings.
  template<typename Iter>
  stream_to(
    transaction_base &, std::string_view table_name, Iter columns_begin,
    Iter columns_end);

  ~stream_to() noexcept;

  [[nodiscard]] operator bool() const noexcept { return not m_finished; }
  [[nodiscard]] bool operator!() const noexcept { return m_finished; }

  /// Complete the operation, and check for errors.
  /** Always call this to close the stream in an orderly fashion, even after
   * an error.  (In the case of an error, abort the transaction afterwards.)
   *
   * The only circumstance where it's safe to skip this is after an error, if
   * you're discarding the entire connection.
   */
  void complete();

  /// Insert a row of data.
  /** Returns a reference to the stream, so you can chain the calls.
   *
   * The @c row can be a tuple, or any type that can be iterated.  Each
   * item becomes a field in the row, in the same order as the columns you
   * specified when creating the stream.
   *
   * If you don't already happen to have your fields in the form of a tuple or
   * container, prefer @c write_values.  It's faster and more convenient.
   */
  template<typename Row> stream_to &operator<<(Row const &row)
  {
    write_row(row);
    return *this;
  }

  /// Stream a `stream_from` straight into a `stream_to`.
  /** This can be useful when copying between different databases.  If the
   * source and the destination are on the same database, you'll get better
   * performance doing it all in a regular query.
   */
  stream_to &operator<<(stream_from &);

  /// Insert a row of data, given in the form of a @c std::tuple or container.
  /** The @c row can be a tuple, or any type that can be iterated.  Each
   * item becomes a field in the row, in the same order as the columns you
   * specified when creating the stream.
   *
   * The preferred way to insert a row is @c write_values.
   */
  template<typename Row> void write_row(Row const &row)
  {
    fill_buffer(row);
    write_buffer();
  }

  /// Insert values as a row.
  /** This is the recommended way of inserting data.  Pass your field values,
   * of any convertible type.
   */
  template<typename... Ts> void write_values(const Ts &... fields)
  {
    fill_buffer(fields...);
    write_buffer();
  }

private:
  bool m_finished = false;

  /// Reusable buffer for a row.  Saves doing an allocation for each row.
  std::string m_buffer;

  /// Reusable buffer for converting/escaping a field.
  std::string m_field_buf;

  /// Write a row of raw text-format data into the destination table.
  void write_raw_line(std::string_view);

  /// Write a row of data from @c m_buffer into the destination table.
  /** Resets the buffer for the next row.
   */
  void write_buffer();

  /// COPY encoding for a null field, plus subsequent separator.
  static constexpr std::string_view null_field{"\\N\t"};

  /// Estimate buffer space needed for a field which is always null.
  template<typename T>
  static std::enable_if_t<nullness<T>::always_null, std::size_t>
  estimate_buffer(T const &)
  {
    return null_field.size();
  }

  /// Estimate buffer space needed for field f.
  /** The estimate is not very precise.  We don't actually know how much space
   * we'll need once the escaping comes in.
   */
  template<typename T>
  static std::enable_if_t<not nullness<T>::always_null, std::size_t>
  estimate_buffer(T const &field)
  {
    return is_null(field) ? null_field.size() : size_buffer(field);
  }

  /// Append escaped version of @c m_field_buf to @c m_buffer, plus a tab.
  void escape_field_to_buffer(std::string_view);

  /// Append string representation for @c f to @c m_buffer.
  /** This is for the general case, where the field may contain a value.
   *
   * Also appends a tab.  The tab is meant to be a separator, not a terminator,
   * so if you write any fields at all, you'll end up with one tab too many
   * at the end of the buffer.
   */
  template<typename Field>
  std::enable_if_t<not nullness<Field>::always_null>
  append_to_buffer(Field const &f)
  {
    // We append each field, terminated by a tab.  That will leave us with
    // one tab too many, assuming we write any fields at all; we remove that
    // at the end.
    if (is_null(f))
    {
      // Easy.  Append null and tab in one go.
      m_buffer.append(null_field);
    }
    else
    {
      // Convert f into m_buffer.

      using traits = string_traits<Field>;
      auto const budget{estimate_buffer(f)};
      auto const offset{m_buffer.size()};

      if constexpr (std::is_arithmetic_v<Field>)
      {
        // Specially optimised for "safe" types, which never need any
        // escaping.  Convert straight into m_buffer.

        // The budget we get from size_buffer() includes room for the trailing
        // zero, which we must remove.  But we're also inserting tabs between
        // fields, so we re-purpose the extra byte for that.
        auto const total{offset + budget};
        m_buffer.resize(total);
        char *const end{traits::into_buf(
          m_buffer.data() + offset, m_buffer.data() + total, f)};
        *(end - 1) = '\t';
        // Shrink to fit.  Keep the tab though.
        m_buffer.resize(static_cast<std::size_t>(end - m_buffer.data()));
      }
      else
      {
        // TODO: Specialise string/string_view/zview to skip to_buf()!
        // This field may need escaping.  First convert the value into
        // m_field_buffer, then escape into its final place.
        m_field_buf.resize(budget);
        escape_field_to_buffer(traits::to_buf(
          m_field_buf.data(), m_field_buf.data() + m_field_buf.size(), f));
      }
    }
  }

  /// Append string representation for a null field to @c m_buffer.
  /** This special case is for types which are always null.
   *
   * Also appends a tab.  The tab is meant to be a separator, not a terminator,
   * so if you write any fields at all, you'll end up with one tab too many
   * at the end of the buffer.
   */
  template<typename Field>
  std::enable_if_t<nullness<Field>::always_null>
  append_to_buffer(Field const &)
  {
    m_buffer.append(null_field);
  }

  /// Write raw COPY line into @c m_buffer, based on a container of fields.
  template<typename Container> void fill_buffer(Container const &c)
  {
    // To avoid unnecessary allocations and deallocations, we run through c
    // twice: once to determine how much buffer space we may need, and once to
    // actually write it into the buffer.
    std::size_t budget{0};
    for (auto const &f : c) budget += estimate_buffer(f);
    m_buffer.reserve(budget);
    for (auto const &f : c) append_to_buffer(f);
  }

  /// Estimate how many buffer bytes we need to write tuple.
  template<typename Tuple, std::size_t... indexes>
  static std::size_t
  budget_tuple(Tuple const &t, std::index_sequence<indexes...>)
  {
    return (estimate_buffer(std::get<indexes>(t)) + ...);
  }

  /// Write tuple of fields to @c m_buffer.
  template<typename Tuple, std::size_t... indexes>
  void append_tuple(Tuple const &t, std::index_sequence<indexes...>)
  {
    (append_to_buffer(std::get<indexes>(t)), ...);
  }

  /// Write raw COPY line into @c m_buffer, based on a tuple of fields.
  template<typename... Elts> void fill_buffer(std::tuple<Elts...> const &t)
  {
    using indexes = std::make_index_sequence<sizeof...(Elts)>;

    m_buffer.reserve(budget_tuple(t, indexes{}));
    append_tuple(t, indexes{});
  }

  void set_up(transaction_base &, std::string_view table_name);
  void set_up(
    transaction_base &, std::string_view table_name,
    std::string const &columns);

  /// Write raw COPY line into @c m_buffer, based on varargs fields.
  template<typename... Ts> void fill_buffer(const Ts &... fields)
  {
    (..., append_to_buffer(fields));
  }
};


template<typename Columns>
inline stream_to::stream_to(
  transaction_base &tb, std::string_view table_name, Columns const &columns) :
        stream_to{tb, table_name, std::begin(columns), std::end(columns)}
{}


template<typename Iter>
inline stream_to::stream_to(
  transaction_base &tb, std::string_view table_name, Iter columns_begin,
  Iter columns_end) :
        namedclass{"stream_to", table_name}, internal::transactionfocus{tb}
{
  set_up(tb, table_name, separated_list(",", columns_begin, columns_end));
}
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
