/* Definition of the pqxx::stream_from class.
 *
 * pqxx::stream_from enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_from instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STREAM_FROM
#define PQXX_H_STREAM_FROM

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include <variant>

#include "pqxx/except.hxx"
#include "pqxx/internal/stream_iterator.hxx"
#include "pqxx/separated_list.hxx"
#include "pqxx/transaction_base.hxx"


namespace pqxx
{
/// Efficiently pull data directly out of a table.
class PQXX_LIBEXPORT stream_from : internal::transactionfocus
{
public:
  stream_from(transaction_base &, std::string_view table_name);
  template<typename Columns>
  stream_from(
    transaction_base &, std::string_view table_name, Columns const &columns);
  template<typename Iter>
  stream_from(
    transaction_base &, std::string_view table_name, Iter columns_begin,
    Iter columns_end);

  ~stream_from() noexcept;

  [[nodiscard]] operator bool() const noexcept { return not m_finished; }
  [[nodiscard]] bool operator!() const noexcept { return m_finished; }

  /// Finish this stream.  Call this before continuing to use the connection.
  /** Consumes all remaining lines, and closes the stream.
   *
   * This may take a while if you're abandoning the stream before it's done, so
   * skip it in error scenarios where you're not planning to use the connection
   * again afterwards.
   */
  void complete();

  bool get_raw_line(std::string &);
  template<typename Tuple> stream_from &operator>>(Tuple &);

  /// Doing this with a @c std::variant is going to be horrifically borked.
  template<typename... Vs>
  stream_from &operator>>(std::variant<Vs...> &) = delete;

  /// Iterate over this stream.  Supports range-based "for" loops.
  /** Produces an input iterator over the stream.
   *
   * Do not call this yourself.  Use it like "for (auto data : stream.iter())".
   */
  template<typename... TYPE>[[nodiscard]] auto iter()
  {
    return pqxx::internal::stream_input_iteration<TYPE...>{*this};
  }

private:
  internal::encoding_group m_copy_encoding =
    internal::encoding_group::MONOBYTE;
  std::string m_current_line;
  bool m_finished = false;
  bool m_retry_line = false;

  void set_up(transaction_base &, std::string_view table_name);
  void set_up(
    transaction_base &, std::string_view table_name,
    std::string const &columns);

  void close();

  bool extract_field(
    std::string const &, std::string::size_type &, std::string &) const;

  template<typename T>
  void extract_value(
    std::string const &line, T &t, std::string::size_type &here,
    std::string &workspace) const;

  template<typename Tuple, std::size_t... I>
  void do_extract(
    const std::string &line, Tuple &t, std::string &workspace,
    std::index_sequence<I...>)
  {
    std::string::size_type here{};
    (extract_value(line, std::get<I>(t), here, workspace), ...);
    if (
      here < line.size() and
      not(here == line.size() - 1 and line[here] == '\n'))
      throw usage_error{"Not all fields extracted from stream_from line"};
  }
};


template<typename Columns>
inline stream_from::stream_from(
  transaction_base &tb, std::string_view table_name, Columns const &columns) :
        stream_from{tb, table_name, std::begin(columns), std::end(columns)}
{}


template<typename Iter>
inline stream_from::stream_from(
  transaction_base &tb, std::string_view table_name, Iter columns_begin,
  Iter columns_end) :
        namedclass{"stream_from", table_name},
        transactionfocus{tb}
{
  set_up(tb, table_name, separated_list(",", columns_begin, columns_end));
}


template<typename Tuple> stream_from &stream_from::operator>>(Tuple &t)
{
  if (m_retry_line or get_raw_line(m_current_line))
  {
    // This is just a scratchpad for functions further down to play with.
    // We allocate it here so that we can keep re-using its buffer, rather
    // than always allocating new ones.
    std::string workspace;
    try
    {
      constexpr auto tsize = std::tuple_size_v<Tuple>;
      using indexes = std::make_index_sequence<tsize>;
      do_extract(m_current_line, t, workspace, indexes{});
      m_retry_line = false;
    }
    catch (...)
    {
      m_retry_line = true;
      throw;
    }
  }
  return *this;
}


template<typename T>
void stream_from::extract_value(
  std::string const &line, T &t, std::string::size_type &here,
  std::string &workspace) const
{
  if (extract_field(line, here, workspace))
    t = from_string<T>(workspace);
  else if constexpr (nullness<T>::has_null)
    t = nullness<T>::null();
  else
    internal::throw_null_conversion(type_name<T>);
}

template<>
void PQXX_LIBEXPORT stream_from::extract_value<std::nullptr_t>(
  std::string const &line, std::nullptr_t &, std::string::size_type &here,
  std::string &workspace) const;
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
