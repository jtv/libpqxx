/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_transactor.h
 *
 *   DESCRIPTION
 *      definition of the Pg::Transactor class.
 *   Pg::Transactor is a framework-style wrapper for safe transactions
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pg_compiler.h"

/* Some transactions may be replayed if their connection fails, until they do 
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
 * execute the transaction code embedded in it (see pg_connection.h).
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

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


namespace Pg
{
class Transaction;

class Transactor
{
public:
  explicit Transactor(PGSTD::string TName="AnonymousTransactor") :	//[t4]
    m_Name(TName) {}

  // Overridable transaction definition.  Will be retried if connection goes
  // bad, but not if an exception is thrown while the connection remains open.
  // The parameter is a dedicated transaction context created to perform this
  // operation.  It is generally recommended that a Transactor modify only
  // itself and this Transaction object from here.
  void operator()(Transaction &);					//[t4]

  // Overridable member functions, called by Connection::Perform() if attempt 
  // to run transaction fails/succeeds, respectively.  Use these to patch up
  // runtime state to match events, if needed, or to report failure conditions.
  // If your OnCommit() function should throw an exception, the actual back-end 
  // transaction will still be committed so the effects on the database remain.
  // The OnAbort() function is not allowed to throw exceptions at all.
  void OnAbort(const char Reason[]) throw () {}				//[]
  void OnCommit() {}							//[]

  PGSTD::string Name() const { return m_Name; }				//[]

private:
  PGSTD::string m_Name;
};

}

