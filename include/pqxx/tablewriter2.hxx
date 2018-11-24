/** Definition of the pqxx::tablewriter2 class.
 *
 * pqxx::tablewriter2 enables optimized batch updates to a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablewriter2.hxx instead.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TABLEWRITER2
#define PQXX_H_TABLEWRITER2

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/tablestream2.hxx"
#include "pqxx/tablereader2"
#include "pqxx/internal/type_utils.hxx"

#include <string>


namespace pqxx
{

/// Efficiently write data directly to a database table.
class PQXX_LIBEXPORT tablewriter2 : public tablestream2
{
  friend class tablereader2; // for operator<<(tablereader2 &)
public:
  tablewriter2(
    transaction_base &,
    const std::string &table_name
  );
  template<typename Columns> tablewriter2(
    transaction_base &,
    const std::string &table_name,
    const Columns& columns
  );
  template<typename Iter> tablewriter2(
    transaction_base &,
    const std::string &table_name,
    Iter columns_begin,
    Iter columns_end
  );

  ~tablewriter2() noexcept;

  void complete() override;

  void write_raw_line(const std::string &);
  template<typename Tuple> tablewriter2 & operator<<(const Tuple &);
  // This is mostly useful for copying data between databases or servers, as
  // executing a query to copy the data within a single database will be much
  // more efficient
  tablewriter2 &operator<<(tablereader2 &);

private:
  void setup(transaction_base &, const std::string &table_name);
  void setup(
    transaction_base &,
    const std::string &table_name,
    const std::string &columns
  );

  void close() override;

};


template<typename Columns> tablewriter2::tablewriter2(
  transaction_base &tb,
  const std::string &table_name,
  const Columns& columns
) : tablewriter2(
  tb,
  table_name,
  std::begin(columns),
  std::end(columns)
)
{}


template<typename Iter> tablewriter2::tablewriter2(
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


namespace internal
{

class PQXX_LIBEXPORT TypedCopyEscaper
{
  static std::string escape(const std::string &);
public:
  template<typename T> std::string operator()(const T* t) const
  {
    return string_traits<T>::is_null(*t) ? "\\N" : escape(to_string(*t));
  }
};

// Explicit specialization so we don't need a string_traits<> for nullptr_t
template<> inline std::string TypedCopyEscaper::operator()<std::nullptr_t>(
  const std::nullptr_t*
) const
{ return "\\N"; }

} // namespace pqxx::internal


template<typename Tuple> tablewriter2 & tablewriter2::operator<<(const Tuple &t)
{
  write_raw_line(separated_list("\t", t, internal::TypedCopyEscaper()));
  return *this;
}

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif
