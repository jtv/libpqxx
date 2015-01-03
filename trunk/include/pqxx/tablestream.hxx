/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/tablestream.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::tablestream class.
 *   pqxx::tablestream provides optimized batch access to a database table
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/tablestream instead.
 *
 * Copyright (c) 2001-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_TABLESTREAM
#define PQXX_H_TABLESTREAM
#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"
#include "pqxx/transaction_base"
namespace pqxx
{
class transaction_base;
/// @deprecated Base class for obsolete tablereader/tablewriter classes.
class PQXX_LIBEXPORT PQXX_NOVTABLE tablestream :
  public internal::transactionfocus
{
public:
  explicit tablestream(transaction_base &Trans,
	      const std::string &Null=std::string());
  virtual ~tablestream() PQXX_NOEXCEPT =0;
  virtual void complete() =0;
protected:
  const std::string &NullStr() const { return m_Null; }
  bool is_finished() const PQXX_NOEXCEPT { return m_Finished; }
  void base_close();
  template<typename ITER>
  static std::string columnlist(ITER colbegin, ITER colend);
private:
  std::string m_Null;
  bool m_Finished;
  tablestream();
  tablestream(const tablestream &);
  tablestream &operator=(const tablestream &);
};
template<typename ITER> inline
std::string tablestream::columnlist(ITER colbegin, ITER colend)
{
  return separated_list(",", colbegin, colend);
}
} // namespace pqxx
#include "pqxx/compiler-internal-post.hxx"
#endif
