/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/dbtransaction.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::dbtransaction abstract base class.
 *   pqxx::dbransaction defines a real transaction on the database
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/dbtransaction instead.
 *
 * Copyright (c) 2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include <string>

#include "pqxx/transaction_base"

namespace pqxx
{

/// Abstract base class responsible for bracketing a backend transaction
/** Use a dbtransaction-derived object such as "work" (transaction<>) to enclose
 * operations on a database in a single "unit of work."  This ensures that the
 * whole series of operations either succeeds as a whole or fails completely.
 * In no case will it leave half-finished work behind in the database.
 *
 * Once processing on a transaction has succeeded and any changes should be
 * allowed to become permanent in the database, call Commit().  If something
 * has gone wrong and the changes should be forgotten, call Abort() instead.
 * If you do neither, an implicit Abort() is executed at destruction time.
 *
 * It is an error to abort a transaction that has already been committed, or to 
 * commit a transaction that has already been aborted.  Aborting an already 
 * aborted transaction or committing an already committed one has been allowed 
 * to make errors easier to deal with.  Repeated aborts or commits have no
 * effect after the first one.
 *
 * Database transactions are not suitable for guarding long-running processes.
 * If your transaction code becomes too long or too complex, please consider
 * ways to break it up into smaller ones.  There's no easy, general way to do
 * this since application-specific considerations become important at this 
 * point.
 *
 * The actual operations for beginning and committing/aborting the backend
 * transaction are implemented by a derived class.  The implementing concrete 
 * class must also call Begin() and End() from its constructors and destructors,
 * respectively, and implement DoExec().
 */
class PQXX_LIBEXPORT dbtransaction : public transaction_base
{
protected:
  explicit dbtransaction(connection_base &C,
			 const PGSTD::string &IsolationString,
      			 const PGSTD::string &NName,
			 const PGSTD::string &CName) :
    transaction_base(C, NName, CName),
    m_StartCmd()
  {
    if (IsolationString != isolation_traits<read_committed>::name())
      m_StartCmd = "SET TRANSACTION ISOLATION LEVEL " + IsolationString;
  }

  /// Start a transaction on the backend and set desired isolation level
  void start_backend_transaction()
  {
    DirectExec("BEGIN", 2);
    if (!startcommand().empty()) DirectExec(startcommand().c_str());
  }

  /// @deprecated This function has changed meanings.  Don't use it.
  const PGSTD::string &startcommand() const { return m_StartCmd; }

#ifdef PQXX_DEPRECATED_HEADERS
  /// @deprecated This function has changed meanings.  Don't use it.
  const PGSTD::string &StartCmd() const { return startcommand(); }
#endif

private:

  /// To be implemented by derived class: start backend transaction
  virtual void do_begin() =0;
  /// Sensible default implemented here: perform query
  virtual result do_exec(const char Query[]);				//[t1]
  /// To be implemented by derived class: commit backend transaction
  virtual void do_commit() =0;
  /// To be implemented by derived class: abort backend transaction
  virtual void do_abort() =0;

  /// Precomputed SQL command to start this transaction
  PGSTD::string m_StartCmd;
};


inline result dbtransaction::do_exec(const char Query[])
{
  result R;
  try
  {
    R = DirectExec(Query);
  }
  catch (const PGSTD::exception &)
  {
    try { abort(); } catch (const PGSTD::exception &) {}
    throw;
  }
  return R;
}

} // namespace pqxx

