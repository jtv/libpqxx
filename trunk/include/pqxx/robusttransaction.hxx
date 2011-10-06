/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/robusttransaction.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::robusttransaction class.
 *   pqxx::robusttransaction is a slower but safer transaction class
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/robusttransaction instead.
 *
 * Copyright (c) 2002-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_ROBUSTTRANSACTION
#define PQXX_H_ROBUSTTRANSACTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/dbtransaction"

#ifdef PQXX_QUIET_DESTRUCTORS
#include "pqxx/errorhandler"
#endif


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */


namespace pqxx
{

/**
 * @addtogroup transaction Transaction classes
 *
 * @{
 */

class PQXX_LIBEXPORT PQXX_NOVTABLE basic_robusttransaction :
  public dbtransaction
{
public:
  /// Isolation level is read_committed by default
  typedef isolation_traits<read_committed> isolation_tag;

  virtual ~basic_robusttransaction() =0;				//[t16]

protected:
  basic_robusttransaction(
	connection_base &C,
	const PGSTD::string &IsolationLevel,
	const PGSTD::string &table_name=PGSTD::string());		//[t16]

private:
  typedef unsigned long IDType;
  IDType m_record_id;
  PGSTD::string m_xid;
  PGSTD::string m_LogTable;
  PGSTD::string m_sequence;
  int m_backendpid;

  virtual void do_begin();						//[t18]
  virtual void do_commit();						//[t16]
  virtual void do_abort();						//[t18]

  void PQXX_PRIVATE CreateLogTable();
  void PQXX_PRIVATE CreateTransactionRecord();
  PGSTD::string PQXX_PRIVATE sql_delete() const;
  void PQXX_PRIVATE DeleteTransactionRecord() throw ();
  bool PQXX_PRIVATE CheckTransactionRecord();
};



/// Slightly slower, better-fortified version of transaction
/** robusttransaction is similar to transaction, but spends more effort (and
 * performance!) to deal with the hopefully rare case that the connection to
 * the backend is lost just as the current transaction is being committed.  In
 * this case, there is no way to determine whether the backend managed to
 * commit the transaction before noticing the loss of connection.
 *
 * In such cases, this class tries to reconnect to the database and figure out
 * what happened.  It will need to store and manage some information (pretty
 * much a user-level transaction log) in the back-end for each and every
 * transaction just on the off chance that this problem might occur.
 * This service level was made optional since you may not want to pay this
 * overhead where it is not necessary.  Certainly the use of this class makes
 * no sense for local connections, or for transactions that read the database
 * but never modify it, or for noncritical database manipulations.
 *
 * Besides being slower, it's theoretically possible that robusttransaction
 * actually fails more instead of less often than a normal transaction.  This is
 * due to the added work and complexity.  What robusttransaction tries to
 * achieve is to be more deterministic, not more successful per se.
 *
 * When a user first uses a robusttransaction in a database, the class will
 * attempt to create a log table there to keep vital transaction-related state
 * information in.  This table, located in that same database, will be called
 * pqxxlog_*user*, where *user* is the PostgreSQL username for that user.  If
 * the log table can not be created, the transaction fails immediately.
 *
 * If the user does not have permission to create the log table, the database
 * administrator may create one for him beforehand, and give ownership (or at
 * least full insert/update rights) to the user.  The table must contain two
 * non-unique fields (which will never be null): "name" (of text type,
 * @c varchar(256) by default) and "date" (of @c timestamp type).  Older
 * versions of robusttransaction also added a unique "id" field; this field is
 * now obsolete and the log table's implicit oids are used instead.  The log
 * tables' names may be made configurable in a future version of libpqxx.
 *
 * The transaction log table contains records describing unfinished
 * transactions, i.e. ones that have been started but not, as far as the client
 * knows, committed or aborted.  This can mean any of the following:
 *
 * <ol>
 * <li> The transaction is in progress.  Since backend transactions can't run
 * for extended periods of time, this can only be the case if the log record's
 * timestamp (compared to the server's clock) is not very old, provided of
 * course that the server's system clock hasn't just made a radical jump.
 * <li> The client's connection to the server was lost, just when the client was
 * committing the transaction, and the client so far has not been able to
 * re-establish the connection to verify whether the transaction was actually
 * completed or rolled back by the server.  This is a serious (and luckily a
 * rare) condition and requires manual inspection of the database to determine
 * what happened.  The robusttransaction will emit clear and specific warnings
 * to this effect, and will identify the log record describing the transaction
 * in question.
 * <li> The transaction was completed (either by commit or by rollback), but the
 * client's connection was durably lost just as it tried to clean up the log
 * record.  Again, robusttransaction will emit a clear and specific warning to
 * tell you about this and request that the record be deleted as soon as
 * possible.
 * <li> The client has gone offline at any time while in one of the preceding
 * states.  This also requires manual intervention, but the client obviously is
 * not able to issue a warning.
 * </ol>
 *
 * It is safe to drop a log table when it is not in use (ie., it is empty or all
 * records in it represent states 2-4 above).  Each robusttransaction will
 * attempt to recreate the table at its next time of use.
 */
template<isolation_level ISOLATIONLEVEL=read_committed>
class robusttransaction : public basic_robusttransaction
{
public:
  typedef isolation_traits<ISOLATIONLEVEL> isolation_tag;

  /// Constructor
  /** Creates robusttransaction of given name
   * @param C Connection that this robusttransaction should live inside.
   * @param Name optional human-readable name for this transaction
   */
  explicit robusttransaction(connection_base &C,
      const PGSTD::string &Name=PGSTD::string()) :
    namedclass(fullname("robusttransaction",isolation_tag::name()), Name),
    basic_robusttransaction(C, isolation_tag::name())
	{ Begin(); }

  virtual ~robusttransaction() throw ()
  {
#ifdef PQXX_QUIET_DESTRUCTORS
    quiet_errorhandler quiet(conn());
#endif
    End();
  }
};

/**
 * @}
 */

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"

#endif

