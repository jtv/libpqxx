/* Definition of the pqxx::stream_from class.
 *
 * pqxx::stream_from enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/stream_from instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_STREAM_FROM
#define PQXX_H_STREAM_FROM

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/util.hxx"

#include <string>


namespace pqxx
{
/// Efficiently pull data directly out of a table.
class PQXX_LIBEXPORT stream_from : internal::transactionfocus
{
public:
  stream_from(
    transaction_base &,
    const std::string &table_name
  );
  template<typename Columns> stream_from(
    transaction_base &,
    const std::string &table_name,
    const Columns& columns
  );
  template<typename Iter> stream_from(
    transaction_base &,
    const std::string &table_name,
    Iter columns_begin,
    Iter columns_end
  );

  ~stream_from() noexcept;

  operator bool() const noexcept { return not m_finished; }
  bool operator!() const noexcept { return m_finished; }

  /// Finish this stream.  Call this before continuing to use the connection.
  /** Consumes all remaining lines, and closes the stream.
   *
   * This may take a while if you're abandoning the stream before it's done, so
   * skip it in error scenarios where you're not planning to use the connection
   * again afterwards.
   */
  void complete();

  bool get_raw_line(std::string &);
  template<typename Tuple> stream_from & operator>>(Tuple &);

private:
  internal::encoding_group m_copy_encoding;
  std::string m_current_line;
  bool m_finished = false;
  bool m_retry_line = false;

  void set_up(transaction_base &, const std::string &table_name);
  void set_up(
    transaction_base &,
    const std::string &table_name,
    const std::string &columns
  );

  void close();

  bool extract_field(
    const std::string &,
    std::string::size_type &,
    std::string &
  ) const;

  template<typename Tuple, std::size_t I> auto tokenize_ith(
    const std::string &,
    Tuple &,
    std::string::size_type,
    std::string &
  ) const -> typename std::enable_if<(
    std::tuple_size<Tuple>::value > I
  )>::type;
  template<typename Tuple, std::size_t I> auto tokenize_ith(
    const std::string &,
    Tuple &,
    std::string::size_type,
    std::string &
  ) const -> typename std::enable_if<(
    std::tuple_size<Tuple>::value <= I
  )>::type;

  template<typename T> void extract_value(
    const std::string &line,
    T& t,
    std::string::size_type &here,
    std::string &workspace
  ) const;
};


template<typename Columns> inline stream_from::stream_from(
  transaction_base &tb,
  const std::string &table_name,
  const Columns& columns
) : stream_from{
  tb,
  table_name,
  std::begin(columns),
  std::end(columns)
} {}


template<typename Iter> inline stream_from::stream_from(
  transaction_base &tb,
  const std::string &table_name,
  Iter columns_begin,
  Iter columns_end
) :
  namedclass{"stream_from", table_name},
  transactionfocus{tb}
{
  set_up(
    tb,
    table_name,
    separated_list(",", columns_begin, columns_end)
  );
}


template<typename Tuple> stream_from &stream_from::operator>>(Tuple &t)
{
  if (m_retry_line or get_raw_line(m_current_line))
  {
    std::string workspace;
    try
    {
      tokenize_ith<Tuple, 0>(m_current_line, t, 0, workspace);
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


template<typename Tuple, std::size_t I> auto stream_from::tokenize_ith(
  const std::string &line,
  Tuple &t,
  std::string::size_type here,
  std::string &workspace
) const -> typename std::enable_if<(
  std::tuple_size<Tuple>::value > I
)>::type
{
  if (here >= line.size())
    throw usage_error{"Too few fields to extract from stream_from line."};

  extract_value(line, std::get<I>(t), here, workspace);
  tokenize_ith<Tuple, I+1>(line, t, here, workspace);
}


template<typename Tuple, std::size_t I> auto stream_from::tokenize_ith(
  const std::string &line,
  Tuple & /* t */,
  std::string::size_type here,
  std::string & /* workspace */
) const -> typename std::enable_if<(
  std::tuple_size<Tuple>::value <= I
)>::type
{
  // Zero-column line may still have a trailing newline
  if (
	here < line.size() and
        not (here == line.size() - 1 and line[here] == '\n'))
    throw usage_error{"Not all fields extracted from stream_from line"};
}


template<typename T> void stream_from::extract_value(
  const std::string &line,
  T &t,
  std::string::size_type &here,
  std::string &workspace
) const
{
  if (extract_field(line, here, workspace)) t = from_string<T>(workspace);
  else if constexpr (string_traits<T>::has_null) t = string_traits<T>::null();
  else internal::throw_null_conversion(type_name<T>);
}

template<> void PQXX_LIBEXPORT stream_from::extract_value<std::nullptr_t>(
  const std::string &line,
  std::nullptr_t&,
  std::string::size_type &here,
  std::string &workspace
) const;
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"
#endif
