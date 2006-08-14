/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/subtransaction.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::subtransaction class.
 *   pqxx::subtransaction is a nested transaction, i.e. one within a transaction
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/subtransaction instead.
 *
 * Copyright (c) 2005-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/dbtransaction"



/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{

/**
 * @addtogroup transaction Transaction classes
 */
/// "Transaction" nested within another transaction
/** A subtransaction can be executed inside a backend transaction, or inside
 * another subtransaction.  This can be useful when, for example, statements in
 * a transaction may harmlessly fail and you don't want them to abort the entire
 * transaction.  Here's an example of how a temporary table may be dropped
 * before re-creating it, without failing if the table did not exist:
 *
 * @code
 * void do_job(connection_base &C)
 * {
 *   const string temptable = "fleetingtable";
 *
 *   // Since we're dealing with a temporary table here, disallow automatic
 *   // recovery of the connection in case it breaks.
 *   C.inhibit_reactivation(true);
 *
 *   work W(C, "do_job");
 *   do_firstpart(W);
 *
 *   // Attempt to delete our temporary table if it already existed
 *   try
 *   {
 *     subtransaction S(W, "droptemp");
 *     W.exec("DROP TABLE " + temptable);
 *   }
 *   catch (const undefined_table &)
 *   {
 *     // Table did not exist.  Which is what we were hoping to achieve anyway.
 *     // Carry on without regrets.
 *   }
 *
 *   W.exec("CREATE TEMP TABLE " + fleetingtable +
 *          "(bar integer, splat varchar)");
 *
 *   do_lastpart(W);
 * }
 * @endcode
 *
 * There are no isolation levels inside a transaction.  They are not needed
 * because all actions within the same backend transaction are always performed
 * sequentially anyway.
 */
class PQXX_LIBEXPORT subtransaction :
  public internal::transactionfocus,
  public dbtransaction
{
public:
  explicit subtransaction(dbtransaction &T,
	const PGSTD::string &Name=PGSTD::string());			//[t88]

private:
  virtual void do_begin();						//[t88]
  virtual void do_commit();						//[t88]
  virtual void do_abort();						//[t88]

  void check_backendsupport() const;

  dbtransaction &m_parent;
};

}


#include "pqxx/compiler-internal-post.hxx"
