/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/transactor.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::transactor class.
 *   pqxx::transactor is a framework-style wrapper for safe transactions
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transactor instead.
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/connection_base"
#include "pqxx/transaction"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */


namespace pqxx
{

/// Wrapper for transactions that automatically restarts them on failure
/** Some transactions may be replayed if their connection fails, until they do 
 * succeed.  These can be encapsulated in a transactor-derived classes.  The 
 * transactor framework will take care of setting up a backend transaction 
 * context for the operation, and of aborting and retrying if its connection 
 * goes bad.
 *
 * The transactor framework also makes it easier for you to do this safely,
 * avoiding typical pitfalls and encouraging programmers to separate their
 * transaction definitions (essentially, business rules implementations) from 
 * their higher-level code (application using those business rules).  The 
 * former go into the transactor-based class.
 *
 * Pass an object of your transactor-based class to connection_base::perform() 
 * to execute the transaction code embedded in it (see the definitions in
 * pqxx/connection_base.hxx).
 *
 * connection_base::Perform() is actually a template, specializing itself to any
 * transactor type you pass to it.  This means you will have to pass it a
 * reference of your object's ultimate static type; runtime polymorphism is
 * not allowed.  Hence the absence of virtual methods in transactor.  The
 * exact methods to be called at runtime *must* be resolved at compile time.
 *
 * Your transactor-derived class must define a copy constructor.  This will be 
 * used to create a "clean" copy of your transactor for every attempt that 
 * Perform() makes to run it.
 */
template<typename TRANSACTION=transaction<read_committed> > 
  class transactor : 
    public PGSTD::unary_function<TRANSACTION, void>
{
public:
  explicit transactor(const PGSTD::string &TName="transactor") :	//[t4]
    m_Name(TName) { }

  /// Overridable transaction definition; insert your database code here
  /** The operation will be retried if connection is lost, but not if an
   * exception is thrown while the connection remains open.
   *
   * Recommended practice is to allow this operator to modify only the
   * transactor itself, and the dedicated transaction object it is passed as an
   * argument.  This is what makes side effects, retrying etc. controllable in
   * the transactor framework.
   * @param T Dedicated transaction context created to perform this operation.
   */
  void operator()(TRANSACTION &T);					//[t4]

  // Overridable member functions, called by connection_base::Perform() if an
  // attempt to run transaction fails/succeeds, respectively, or if the 
  // connection is lost at just the wrong moment, goes into an indeterminate 
  // state.  Use these to patch up runtime state to match events, if needed, or
  // to report failure conditions.

  // TODO: Rename OnAbort()--is there a compatible way?
  /// Optional overridable function to be called if transaction is aborted
  /** This need not imply complete failure; the transactor will automatically
   * retry the operation a number of times before giving up.  OnAbort() will be
   * called for each of the failed attempts.
   *
   * @param msg Error string describing why the transaction failed
   */
  void OnAbort(const char msg[]) throw () {}				//[t13]

  // TODO: Rename OnCommit()--is there a compatible way?
  /// Optional overridable function to be called when committing the transaction
  /** If your OnCommit() throws an exception, the actual back-end transaction
   * will remain committed, so any changes in the database remain.
   */
  void OnCommit() {}							//[t6]

  // TODO: Rename OnCommit()--is there a compatible way?
  /// Overridable function to be called when "in doubt" about success
  /** This may happen if the connection to the backend is lost while attempting
   * to commit.  In that case, the backend may have committed the transaction
   * but is unable to confirm this to the frontend; or the backend may have
   * failed completely, causing the transaction to be aborted.  The best way to
   * deal with this situation is to wave red flags in the user's face and ask
   * him to investigate.
   *
   * Also, the robusttransaction class is intended to reduce the chances of this
   * error occurring.
   */
  void OnDoubt() throw () {}						//[t13]

  // TODO: Rename Name()--is there a compatible way?
  /// The transactor's name.
  PGSTD::string Name() const { return m_Name; }				//[t13]

private:
  PGSTD::string m_Name;
};


}


/** Invoke a transactor, making at most Attempts attempts to perform the
 * encapsulated code on the database.  If the code throws any exception other
 * than broken_connection, it will be aborted right away.
 *
 * Take care: no member functions will be invoked on the original transactor you
 * pass into the function.  It only serves as a prototype for the transaction to
 * be performed.  The perform() function will copy-construct transactors from
 * the original you passed in, executing the copies only and retaining a "clean"
 * original.
 */
template<typename TRANSACTOR> 
inline void pqxx::connection_base::perform(const TRANSACTOR &T,
                                           int Attempts)
{
  if (Attempts <= 0) return;

  bool Done = false;

  // Make attempts to perform T
  // TODO: Differentiate between db-related exceptions and other exceptions?
  do
  {
    --Attempts;

    // Work on a copy of T2 so we can restore the starting situation if need be
    TRANSACTOR T2(T);
    try
    {
      typename TRANSACTOR::argument_type X(*this, T2.Name());
      T2(X);
      X.commit();
      Done = true;
    }
    catch (const in_doubt_error &)
    {
      // Not sure whether transaction went through or not.  The last thing in
      // the world that we should do now is retry.
      T2.OnDoubt();
      throw;
    }
    catch (const PGSTD::exception &e)
    {
      // Could be any kind of error.  
      T2.OnAbort(e.what());
      if (Attempts <= 0) throw;
      continue;
    }
    catch (...)
    {
      // Don't try to forge ahead if we don't even know what happened
      T2.OnAbort("Unknown exception");
      throw;
    }

    T2.OnCommit();
  } while (!Done);
}


