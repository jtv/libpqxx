/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/nontransaction.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::nontransaction class.
 *   pqxx::nontransaction provides nontransactional database access
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/nontransaction instead.
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/transaction_base"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{
class nontransaction;

/// Human-readable class names for use by unique template.
template<> inline PGSTD::string Classname(const nontransaction *) 
{ 
  return "nontransaction"; 
}



/// Simple "transaction" class offering no transactional integrity.
/**
 * nontransaction, like transaction or any other transaction_base-derived class,
 * provides access to a database through a connection.  Unlike its siblings,
 * however, nontransaction does not maintain any kind of transactional 
 * integrity.  This may be useful eg. for read-only access to the database that
 * does not require a consistent, atomic view on its data; or for operations
 * that are not allowed within a backend transaction, such as creating tables.
 * For queries that update the database, however, a real transaction is likely
 * to be faster unless the transaction consists of only a single record update.
 * As a side effect, you can keep a nontransaction open for as long as you like.
 * Actual back-end transactions are limited in lifespan, and will sometimes fail
 * just because they took too long to execute.  This will not happen with a
 * nontransaction.  It may need to restore its connection, but it won't abort as
 * a result.
 *
 * Any query executed in a nontransaction is committed immediately, and neither
 * Commit() nor Abort() has any effect.
 *
 * Database features that require a backend transaction, such as cursors or
 * large objects, will not work in a nontransaction.
 */
class PQXX_LIBEXPORT nontransaction : public transaction_base
{
public:
  /// Constructor.
  /** Create a "dummy" transaction.
   * @param C the connection that this "transaction" will operate on.
   * @param NName an optional name for the transaction, beginning with a letter
   * and containing only letters and digits.
   */
  explicit nontransaction(connection_base &C, 
		          const PGSTD::string &NName=PGSTD::string()) :	//[t14]
    transaction_base(C, NName) { Begin(); }

  virtual ~nontransaction();						//[t14]

private:
  virtual void do_begin() {}						//[t14]
  virtual result do_exec(const char C[]);				//[t14]
  virtual void do_commit() {}						//[t14]
  virtual void do_abort() {}						//[t14]
};


}


