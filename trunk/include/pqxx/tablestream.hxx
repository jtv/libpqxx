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
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
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
	      const PGSTD::string &Null=PGSTD::string());
  virtual ~tablestream() throw () =0;
  virtual void complete() =0;
protected:
  const PGSTD::string &NullStr() const { return m_Null; }
  bool is_finished() const throw () { return m_Finished; }
  void base_close();
  template<typename ITER>
  static PGSTD::string columnlist(ITER colbegin, ITER colend);
private:
  PGSTD::string m_Null;
  bool m_Finished;
  tablestream();
  tablestream(const tablestream &);
  tablestream &operator=(const tablestream &);
};
template<typename ITER> inline
PGSTD::string tablestream::columnlist(ITER colbegin, ITER colend)
{
  return separated_list(",", colbegin, colend);
}
} // namespace pqxx
#include "pqxx/compiler-internal-post.hxx"
#endif
