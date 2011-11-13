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
 * Copyright (c) 2001-2008, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_TRANSACTOR
#define PQXX_H_TRANSACTOR

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/connection_base"
#include "pqxx/transaction"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{

/// Wrapper for transactions that automatically restarts them on failure
/**
 * @addtogroup transactor Transactor framework
 *
 * Some transactions may be replayed if their connection fails, until they do
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
 * connection_base::perform() is actually a template, specializing itself to any
 * transactor type you pass to it.  This means you will have to pass it a
 * reference of your object's ultimate static type; runtime polymorphism is
 * not allowed.  Hence the absence of virtual methods in transactor.  The
 * exact methods to be called at runtime *must* be resolved at compile time.
 *
 * Your transactor-derived class must define a copy constructor.  This will be
 * used to create a "clean" copy of your transactor for every attempt that
 * perform() makes to run it.
 */
template<typename TRANSACTION=transaction<read_committed> >
  class transactor :
    public PGSTD::unary_function<TRANSACTION, void>
{
public:
  explicit transactor(const PGSTD::string &TName="transactor") :	//[t4]
    m_Name(TName) { }

  /// Overridable transaction definition; insert your database code here
  /** The operation will be retried if the connection to the backend is lost or
   * the operation fails, but not if the connection is broken in such a way as
   * to leave the library in doubt as to whether the operation succeeded.  In
   * that case, an in_doubt_error will be thrown.
   *
   * Recommended practice is to allow this operator to modify only the
   * transactor itself, and the dedicated transaction object it is passed as an
   * argument.  This is what makes side effects, retrying etc. controllable in
   * the transactor framework.
   * @param T Dedicated transaction context created to perform this operation.
   */
  void operator()(TRANSACTION &T);					//[t4]

  // Overridable member functions, called by connection_base::perform() if an
  // attempt to run transaction fails/succeeds, respectively, or if the
  // connection is lost at just the wrong moment, goes into an indeterminate
  // state.  Use these to patch up runtime state to match events, if needed, or
  // to report failure conditions.

  /// Optional overridable function to be called if transaction is aborted
  /** This need not imply complete failure; the transactor will automatically
   * retry the operation a number of times before giving up.  on_abort() will be
   * called for each of the failed attempts.
   *
   * One parameter is passed in by the framework: an error string describing why
   * the transaction failed.  This will also be logged to the connection's
   * notice processor.
   */
  void on_abort(const char[]) throw () {}				//[t13]

  /// Optional overridable function to be called after successful commit
  /** If your on_commit() throws an exception, the actual back-end transaction
   * will remain committed, so any changes in the database remain regardless of
   * how this function terminates.
   */
  void on_commit() {}							//[t7]

  /// Overridable function to be called when "in doubt" about outcome
  /** This may happen if the connection to the backend is lost while attempting
   * to commit.  In that case, the backend may have committed the transaction
   * but is unable to confirm this to the frontend; or the transaction may have
   * failed, causing it to be rolled back, but again without acknowledgement to
   * the client program.  The best way to deal with this situation is typically
   * to wave red flags in the user's face and ask him to investigate.
   *
   * The robusttransaction class is intended to reduce the chances of this
   * error occurring, at a certain cost in performance.
   * @see robusttransaction
   */
  void on_doubt() throw () {}						//[t13]

  // TODO: Rename Name()--is there a compatible way?
  /// The transactor's name.
  PGSTD::string Name() const { return m_Name; }				//[t13]

private:
  PGSTD::string m_Name;
};


}


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
      T2.on_doubt();
      throw;
    }
    catch (const PGSTD::exception &e)
    {
      // Could be any kind of error.
      T2.on_abort(e.what());
      if (Attempts <= 0) throw;
      continue;
    }
    catch (...)
    {
      // Don't try to forge ahead if we don't even know what happened
      T2.on_abort("Unknown exception");
      throw;
    }

    T2.on_commit();
  } while (!Done);
}


#include "pqxx/compiler-internal-post.hxx"

#endif

