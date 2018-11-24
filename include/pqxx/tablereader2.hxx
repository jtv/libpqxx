/** Definition of the pqxx::tablereader2 class.
 *
 * pqxx::tablereader2 enables optimized batch reads from a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablereader2 instead.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TABLEREADER2
#define PQXX_H_TABLEREADER2

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/tablestream2.hxx"
#include "pqxx/internal/type_utils.hxx"

#include <string>


namespace pqxx
{

/// Efficiently pull data directly out of a table.
class PQXX_LIBEXPORT tablereader2 : public tablestream2
{
  friend class tablewriter2; // for tablewriter2::operator<<(tablereader2 &)
public:
  tablereader2(
    transaction_base &,
    const std::string &table_name
  );
  template<typename Columns> tablereader2(
    transaction_base &,
    const std::string &table_name,
    const Columns& columns
  );
  template<typename Iter> tablereader2(
    transaction_base &,
    const std::string &table_name,
    Iter columns_begin,
    Iter columns_end
  );

  ~tablereader2() noexcept;

  virtual void complete() override;

  bool get_raw_line(std::string &);
  template<typename Tuple> tablereader2 & operator>>(Tuple &);

private:
  internal::encoding_group m_copy_encoding;
  std::string m_current_line;
  bool m_retry_line;

  void setup(transaction_base &, const std::string &table_name);
  void setup(
    transaction_base &,
    const std::string &table_name,
    const std::string &columns
  );

  virtual void close() override;

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


template<typename Columns> tablereader2::tablereader2(
  transaction_base &tb,
  const std::string &table_name,
  const Columns& columns
) : tablereader2(
  tb,
  table_name,
  std::begin(columns),
  std::end(columns)
) {}


template<typename Iter> tablereader2::tablereader2(
  transaction_base &tb,
  const std::string &table_name,
  Iter columns_begin,
  Iter columns_end
) :
  namedclass("tablereader2", table_name),
  tablestream2(tb)
{
  setup(
    tb,
    table_name,
    columnlist(columns_begin, columns_end)
  );
}


template<typename Tuple> tablereader2 & tablereader2::operator>>(
  Tuple &t
)
{
  if (m_retry_line || get_raw_line(m_current_line))
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


template<typename Tuple, std::size_t I> auto tablereader2::tokenize_ith(
  const std::string &line,
  Tuple &t,
  std::string::size_type here,
  std::string &workspace
) const -> typename std::enable_if<(
  std::tuple_size<Tuple>::value > I
)>::type
{
  if (here < line.size())
  {
    extract_value(line, std::get<I>(t), here, workspace);
    tokenize_ith<Tuple, I+1>(line, t, here, workspace);
  }
  else
    throw usage_error{"Too few fields to extract from tablereader2 line"};
}


template<typename Tuple, std::size_t I> auto tablereader2::tokenize_ith(
  const std::string &line,
  Tuple &t,
  std::string::size_type here,
  std::string &workspace
) const -> typename std::enable_if<(
  std::tuple_size<Tuple>::value <= I
)>::type
{
  // Zero-column line may still have a trailing newline
  if (here < line.size() && !(here == line.size() - 1 && line[here] == '\n'))
    throw usage_error{"Not all fields extracted from tablereader2 line"};
}


template<typename T> void tablereader2::extract_value(
  const std::string &line,
  T& t,
  std::string::size_type &here,
  std::string &workspace
) const
{
  if (extract_field(line, here, workspace))
    from_string<T>(workspace, t);
  else
    t = internal::null_value<T>();
}

template<> void tablereader2::extract_value<std::nullptr_t>(
  const std::string &line,
  std::nullptr_t&,
  std::string::size_type &here,
  std::string &workspace
) const;

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif
