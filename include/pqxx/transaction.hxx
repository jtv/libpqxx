/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transaction.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::transaction class.
 *   pqxx::transaction represents a standard database transaction
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transaction instead.
 *
 * Copyright (c) 2001-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_TRANSACTION
#define PQXX_H_TRANSACTION

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
//@{

class PQXX_LIBEXPORT basic_transaction : public dbtransaction
{
protected:
  basic_transaction(							//[t1]
	connection_base &C,
	const std::string &IsolationLevel,
	readwrite_policy);

private:
  virtual void do_commit() PQXX_OVERRIDE;				//[t1]
};


/// Standard back-end transaction, templatized on isolation level
/** This is the type you'll normally want to use to represent a transaction on
 * the database.
 *
 * While you may choose to create your own transaction object to interface to
 * the database backend, it is recommended that you wrap your transaction code
 * into a transactor code instead and let the transaction be created for you.
 * @see pqxx/transactor.hxx
 *
 * If you should find that using a transactor makes your code less portable or
 * too complex, go ahead, create your own transaction anyway.
 *
 * Usage example: double all wages
 *
 * @code
 * extern connection C;
 * work T(C);
 * try
 * {
 *   T.exec("UPDATE employees SET wage=wage*2");
 *   T.commit();	// NOTE: do this inside try block
 * }
 * catch (const std::exception &e)
 * {
 *   cerr << e.what() << endl;
 *   T.abort();		// Usually not needed; same happens when T's life ends.
 * }
 * @endcode
 */
template<
	isolation_level ISOLATIONLEVEL=read_committed,
	readwrite_policy READWRITE=read_write>
class transaction : public basic_transaction
{
public:
  typedef isolation_traits<ISOLATIONLEVEL> isolation_tag;

  /// Create a transaction
  /**
   * @param C Connection for this transaction to operate on
   * @param TName Optional name for transaction; must begin with a letter and
   * may contain letters and digits only
   */
  explicit transaction(connection_base &C, const std::string &TName):	//[t1]
    namedclass(fullname("transaction", isolation_tag::name()), TName),
    basic_transaction(C, isolation_tag::name(), READWRITE)
	{ Begin(); }

  explicit transaction(connection_base &C) :				//[t1]
    namedclass(fullname("transaction", isolation_tag::name())),
    basic_transaction(C, isolation_tag::name(), READWRITE)
	{ Begin(); }

  virtual ~transaction() PQXX_NOEXCEPT				      { End(); }
};


template<
	isolation_level ISOLATIONLEVEL=read_committed,
	readwrite_policy READWRITE=read_write>
class timed_transaction : public basic_transaction
{
public:
  typedef isolation_traits<ISOLATIONLEVEL> isolation_tag;

  /// Create a transaction
  /**
   * @param C Connection for this transaction to operate on
   * @param TName Optional name for transaction; must begin with a letter and
   * may contain letters and digits only
   */
  explicit timed_transaction(connection_base &C, const std::string &TName, long Timeout):
    namedclass(fullname("timed_transaction",isolation_tag::name()), TName),
    basic_transaction(C, isolation_tag::name(), READWRITE),
    m_Timeout(Timeout),
    m_C(C)
	{ Begin(); }

  explicit timed_transaction(connection_base &C, int Timeout) :
    namedclass(fullname("timed_transaction",isolation_tag::name())),
    basic_transaction(C, isolation_tag::name(), READWRITE),
    m_Timeout(Timeout),
    m_C(C)
	{ Begin(); }

  virtual ~timed_transaction() throw ()
  {
#ifdef PQXX_QUIET_DESTRUCTORS
    internal::disable_noticer Quiet(conn());
#endif
    End();
  }

  void do_begin();
  result do_exec(const char Query[]);
  void do_commit();
  void do_abort();

private:
  long m_Timeout;
  connection_base &m_C;
};

typedef timed_transaction<> timed_work;

/// Bog-standard, default transaction type
typedef transaction<> work;

/// Read-only transaction
typedef transaction<read_committed, read_only> read_transaction;

//@}

}


template<pqxx::isolation_level ISOLATIONLEVEL, pqxx::readwrite_policy READWRITE>
inline void pqxx::timed_transaction<ISOLATIONLEVEL, READWRITE>::do_begin()
{
  try {
    DirectExec("BEGIN", 0, m_Timeout);
  }
  catch (const transaction_timeout &) {
    try { abort(); m_C.deactivate(); } catch (const std::exception &) {}
    throw;
  }
}

template<pqxx::isolation_level ISOLATIONLEVEL, pqxx::readwrite_policy READWRITE>
inline pqxx::result pqxx::timed_transaction<ISOLATIONLEVEL, READWRITE>::do_exec(const char Query[])
{
    try
    {
      return DirectExec(Query, 0, m_Timeout);
    }
    catch (const transaction_timeout &) {
      try { abort(); m_C.deactivate(); } catch (const std::exception &) {}
      throw;
    }
    catch (const std::exception &)
    {
      try { abort(); } catch (const std::exception &) {}
      throw;
    }
}

template<pqxx::isolation_level ISOLATIONLEVEL, pqxx::readwrite_policy READWRITE>
inline void pqxx::timed_transaction<ISOLATIONLEVEL, READWRITE>::do_commit()
{
  try
  {
    DirectExec(internal::sql_commit_work, 0, m_Timeout);
  }
  catch (const transaction_timeout &) {
    try { abort(); m_C.deactivate(); } catch (const std::exception &) {}
    throw;
  }
  catch (const std::exception &e)
  {
    if (!conn().is_open()) {
      // We've lost the connection while committing.  There is just no way of
      // telling what happened on the other end.  >8-O
      process_notice(e.what() + std::string("\n"));

      const std::string Msg = "WARNING: "
              "Connection lost while committing transaction "
              "'" + name() + "'. "
              "There is no way to tell whether the transaction succeeded "
              "or was aborted except to check manually.";

      process_notice(Msg + "\n");
      throw in_doubt_error(Msg);
    }
    else {
      // Commit failed--probably due to a constraint violation or something
      // similar.
      throw;
    }
  }
}

template<pqxx::isolation_level ISOLATIONLEVEL, pqxx::readwrite_policy READWRITE>
inline void pqxx::timed_transaction<ISOLATIONLEVEL, READWRITE>::do_abort()
{
  try {
    reactivation_avoidance_clear();
    DirectExec(internal::sql_rollback_work, 0, m_Timeout);
  } catch (const transaction_timeout &) {
    try { m_C.deactivate(); } catch (const std::exception &) {}
    throw;
  }
}


#include "pqxx/compiler-internal-post.hxx"

#endif

