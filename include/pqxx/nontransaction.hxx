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
 * Copyright (c) 2002-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_NONTRANSACTION
#define PQXX_H_NONTRANSACTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/connection_base"
#include "pqxx/result"
#include "pqxx/transaction_base"

/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */


namespace pqxx
{

/// Simple "transaction" class offering no transactional integrity.
/**
 * @addtogroup transaction Transaction classes
 *
 * nontransaction, like transaction or any other transaction_base-derived class,
 * provides access to a database through a connection.  Unlike its siblings,
 * however, nontransaction does not maintain any kind of transactional
 * integrity.  This may be useful eg. for read-only access to the database that
 * does not require a consistent, atomic view on its data; or for operations
 * that are not allowed within a backend transaction, such as creating tables.
 *
 * For queries that update the database, however, a real transaction is likely
 * to be faster unless the transaction consists of only a single record update.
 *
 * Also, you can keep a nontransaction open for as long as you like.  Actual
 * back-end transactions are limited in lifespan, and will sometimes fail just
 * because they took too long to execute or were left idle for too long.  This
 * will not happen with a nontransaction (although the connection may still time
 * out, e.g. when the network is unavailable for a very long time).
 *
 * Any query executed in a nontransaction is committed immediately, and neither
 * commit() nor abort() has any effect.
 *
 * Database features that require a backend transaction, such as cursors or
 * large objects, will not work in a nontransaction.
 */
class PQXX_LIBEXPORT nontransaction : public transaction_base
{
public:
  /// Constructor.
  /** Create a "dummy" transaction.
   * @param C Connection that this "transaction" will operate on.
   * @param Name Optional name for the transaction, beginning with a letter
   * and containing only letters and digits.
   */
  explicit nontransaction(connection_base &C,
		          const std::string &Name=std::string()) :	//[t14]
    namedclass("nontransaction", Name), transaction_base(C) { Begin(); }

  virtual ~nontransaction();						//[t14]

private:
  virtual void do_begin() PQXX_OVERRIDE {}				//[t14]
  virtual result do_exec(const char C[]) PQXX_OVERRIDE;			//[t14]
  virtual void do_commit() PQXX_OVERRIDE {}				//[t14]
  virtual void do_abort() PQXX_OVERRIDE {}				//[t14]
};


} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"

#endif

