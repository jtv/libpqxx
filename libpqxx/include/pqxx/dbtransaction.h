/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/dbtransaction.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::dbtransaction abstract base class.
 *   pqxx::dbransaction defines a real transaction on the database
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_DBTRANSACTION_H
#define PQXX_DBTRANSACTION_H

#include <string>

#include "pqxx/transaction_base.h"

namespace pqxx
{

/// Abstract base class responsible for bracketing a backend transaction
/** Use a dbtransaction-derived object such as a transaction<> to enclose
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
      			 const PGSTD::string &NName) :
    transaction_base(C, NName),
    m_StartCmd("START TRANSACTION ISOLATION LEVEL " + IsolationString)
  {}

  /// The SQL command needed to start this type of transaction
  const PGSTD::string &StartCmd() const { return m_StartCmd; }

private:
  /// To be implemented by derived class: start backend transaction
  virtual void DoBegin() =0;
  /// To be implemented by derived implementation class: perform query
  virtual result DoExec(const char Query[]) =0;
  /// To be implemented by derived class: commit backend transaction
  virtual void DoCommit() =0;
  /// To be implemented by derived class: abort backend transaction
  virtual void DoAbort() =0;

  /// Precomputed SQL command to start this transaction
  PGSTD::string m_StartCmd;
};

/// Human-readable class name for use by unique
template<> inline PGSTD::string Classname(const dbtransaction *) 
{ 
  return "dbtransaction"; 
}

}

#endif

