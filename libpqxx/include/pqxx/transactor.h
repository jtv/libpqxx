/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transactor.h
 *
 *   DESCRIPTION
 *      definition of the pqxx::Transactor class.
 *   pqxx::Transactor is a framework-style wrapper for safe transactions
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_TRANSACTOR_H
#define PQXX_TRANSACTOR_H

#include <string>

#include "pqxx/compiler.h"


/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace pqxx
{
class Transaction;

/// Wrapper for transactions that automatically restarts them on failure.
/** Some transactions may be replayed if their connection fails, until they do 
 * succeed.  These can be encapsulated in a Transactor-derived classes.  The 
 * Transactor framework will take care of setting up a backend transaction 
 * context for the operation, and of aborting and retrying if its connection 
 * goes bad.
 *
 * The Transactor framework also makes it easier for you to do this safely,
 * avoiding typical pitfalls and encouraging programmers to separate their
 * transaction definitions (essentially, business rules implementations) from 
 * their higher-level code (application using those business rules).  The 
 * former go into the Transactor-based class.
 *
 * Pass an object of your Transactor-based class to Connection::Perform() to
 * execute the transaction code embedded in it (see pqxx/connection.h).
 *
 * Connection::Perform() is actually a template, specializing itself to any
 * Transactor type you pass to it.  This means you will have to pass it a
 * reference of your object's ultimate static type; runtime polymorphism is
 * not allowed.  Hence the absence of virtual methods in Transactor.  The
 * exact methods to be called at runtime *must* be resolved at compile time.
 *
 * Your Transactor-derived class must define a copy constructor.  This will be 
 * used to create a "clean" copy of your transactor for every attempt that 
 * Perform() makes to run it.
 */
class PQXX_LIBEXPORT Transactor
{
public:
  explicit Transactor(const PGSTD::string &TName="AnonymousTransactor") ://[t4]
    m_Name(TName) {}

  /// Define transaction class to use as a wrapper for this code.  
  /** Select the quality of service for your transactor by overriding this in
   * your derived class.
   */
  typedef Transaction argument_type;

  /// Required to make Transactor adaptable
  typedef void result_type;

  /// Overridable transaction definition.
  /** Will be retried if connection goes bad, but not if an exception is thrown 
   * while the connection remains open.
   * @param T a dedicated transaction context created to perform this 
   * operation.  It is generally recommended that a Transactor modify only 
   * itself and T from inside this operator.
   */
  void operator()(argument_type &T);					//[t4]

  // Overridable member functions, called by Connection::Perform() if attempt 
  // to run transaction fails/succeeds, respectively, or if the connection is
  // lost at just the wrong moment, goes into an indeterminate state.  Use 
  // these to patch up runtime state to match events, if needed, or to report 
  // failure conditions.

  /// Overridable function to be called if transaction is aborted.
  /** This need not imply complete failure; the Transactor will automatically
   * retry the operation a number of times before giving up.  OnAbort() will be
   * called for each of the failed attempts.
   * The Reason argument is an error string describing why the transaction 
   * failed. 
   */
  void OnAbort(const char /*Reason*/[]) throw () {}			//[t13]

  /// Overridable function to be called when committing the transaction. 
  /** If your OnCommit() throws an exception, the actual back-end transaction
   * will remain committed, so any changes in the database remain.
   */
  void OnCommit() {}							//[t6]

  /// Overridable function to be called when going into limbo between committing
  /// and aborting.
  /** This may happen if the connection to the backend is lost while attempting
   * to commit.  In that case, the backend may have committed the transaction
   * but is unable to confirm this to the frontend; or the backend may have
   * failed completely, causing the transaction to be aborted.  The best way to
   * deal with this situation is to wave red flags in the user's face and ask
   * him to investigate.
   * Also, the RobustTransaction class is intended to reduce the chances of this
   * error occurring.
   */
  void OnDoubt() throw () {}						//[t13]

  /// The Transactor's name.
  PGSTD::string Name() const { return m_Name; }				//[t13]

private:
  PGSTD::string m_Name;
};

}

#endif

