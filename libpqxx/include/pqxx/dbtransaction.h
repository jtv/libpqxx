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
/** The actual operations for beginning and committing/aborting the backend
 * transaction are implemented by a derived class.  The implementing concrete 
 * class must also call Begin() and End() from its constructors and destructors,
 * respectively, and implement DoExec().
 */
class PQXX_LIBEXPORT dbtransaction : public transaction_base
{
protected:
  explicit dbtransaction(connection_base &C,
			 const PGSTD::string &IsolationString,
      			 const PGSTD::string &NName) :			//[]
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

