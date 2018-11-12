/** Definition of the pqxx::tablestream2 class.
 *
 * pqxx::tablestream2 provides optimized batch access to a database table.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablestream2 instead.
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TABLESTREAM2
#define PQXX_H_TABLESTREAM2

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/transaction_base.hxx"
#include "pqxx/util.hxx"

#include <string>


namespace pqxx
{

class PQXX_LIBEXPORT PQXX_NOVTABLE tablestream2 :
  public internal::transactionfocus
{
public:
  explicit tablestream2(transaction_base &);
  virtual ~tablestream2() noexcept;
  virtual void complete() = 0;
  operator bool() const noexcept;
  bool operator!() const noexcept;
protected:
  bool m_finished;
  virtual void close();
  template<typename C> static std::string columnlist(const C &);
  template<typename I> static std::string columnlist(I begin, I end);
private:
  tablestream2();
  tablestream2(const tablestream2&);
  tablestream2 & operator=(const tablestream2 &);
};

template<typename C> std::string tablestream2::columnlist(const C &c)
{
  return columnlist(std::begin(c), std::end(c));
}

template<typename I> std::string tablestream2::columnlist(I begin, I end)
{
  return separated_list(",", begin, end);
}

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif


