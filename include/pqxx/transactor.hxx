/* Transactor framework, a wrapper for safely retryable transactions.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transactor instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_TRANSACTOR
#define PQXX_H_TRANSACTOR

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/connection.hxx"
#include "pqxx/transaction.hxx"


// Methods tested in eg. test module test01 are marked with "//[t01]".

namespace pqxx
{
/**
 * @defgroup transactor Transactor framework
 *
 * Sometimes a transaction can fail for completely transient reasons, such as a
 * conflict with another transaction in SERIALIZABLE isolation.  The right way
 * to handle those failures is often just to re-run the transaction from
 * scratch.
 *
 * For example, your REST API might be handling each HTTP request in its own
 * database transaction, and if this kind of transient failure happens, you
 * simply want to "replay" the whole request, in a fresh transaction.
 *
 * You won't necessarily want to execute the exact same SQL commands with the
 * exact same data.  Some of your SQL statements may depend on state that can
 * vary between retries.  Data in the database may already have changed, for
 * instance.  So instead of dumbly replaying the SQL, you re-run the same
 * application code that produced those SQL commands, from the start.
 *
 * The transactor framework makes it a little easier for you to do this safely,
 * and avoid typical pitfalls.  You encapsulate the work that you want to do
 * into a callable that you pass to the @c perform function.
 *
 * Here's how it works.  You write your transaction code as a lambda or
 * function, which creates its own transaction object, does its work, and
 * commits at the end.  You pass that callback to @c pqxx::perform, which runs
 * it for you.
 *
 * If there's a failure inside your callback, there will be an exception.  Your
 * transaction object goes out of scope and gets destroyed, so that it aborts
 * implicitly.  Seeing this, @c perform tries running your callback again.  It
 * stops doing that when the callback succeeds, or when it has failed too many
 * times, or when there's an error that leaves the database in an unknown
 * state, such as a lost connection just while we're waiting for the database
 * to confirm a commit.  It all depends on the type of exception.
 *
 * The callback takes no arguments.  If you're using lambdas, the easy way to
 * pass arguments is for the lambda to "capture" them from your variables.  Or,
 * if you're using functions, you may want to use @c std::bind.
 *
 * Once your callback succeeds, it can return a result, and @c perform will
 * return that result back to you.
 */
//@{

/// Simple way to execute a transaction with automatic retry.
/**
 * Executes your transaction code as a callback.  Repeats it until it completes
 * normally, or it throws an error other than the few libpqxx-generated
 * exceptions that the framework understands, or after a given number of failed
 * attempts, or if the transaction ends in an "in-doubt" state.
 *
 * (An in-doubt state is one where libpqxx cannot determine whether the server
 * finally committed a transaction or not.  This can happen if the network
 * connection to the server is lost just while we're waiting for its reply to
 * a "commit" statement.  The server may have completed the commit, or not, but
 * it can't tell you because there's no longer a connection.
 *
 * Using this still takes a bit of care.  If your callback makes use of data
 * from the database, you'll probably have to query that data within your
 * callback.  If the attempt to perform your callback fails, and the framework
 * tries again, you'll be in a new transaction and the data in the database may
 * have changed under your feet.
 *
 * Also be careful about changing variables or data structures from within
 * your callback.  The run may still fail, and perhaps get run again.  The
 * ideal way to do it (in most cases) is to return your result from your
 * callback, and change your program's data state only after @c perform
 * completes successfully.
 *
 * @param callback Transaction code that can be called with no arguments.
 * @param attempts Maximum number of times to attempt performing callback.
 *	Must be greater than zero.
 * @return Whatever your callback returns.
 */
template<typename TRANSACTION_CALLBACK>
inline auto perform(const TRANSACTION_CALLBACK &callback, int attempts=3)
  -> decltype(callback())
{
  if (attempts <= 0)
    throw std::invalid_argument{
	"Zero or negative number of attempts passed to pqxx::perform()."};

  for (; attempts > 0; --attempts)
  {
    try
    {
      return callback();
    }
    catch (const in_doubt_error &)
    {
      // Not sure whether transaction went through or not.  The last thing in
      // the world that we should do now is try again!
      throw;
    }
    catch (const statement_completion_unknown &)
    {
      // Not sure whether our last statement succeeded.  Don't risk running it
      // again.
      throw;
    }
    catch (const broken_connection &)
    {
      // Connection failed.  May be worth retrying, if the transactor opens its
      // own connection.
      if (attempts <= 1) throw;
      continue;
    }
    catch (const transaction_rollback &)
    {
      // Some error that may well be transient, such as serialization failure
      // or deadlock.  Worth retrying.
      if (attempts <= 1) throw;
      continue;
    }
  }
  throw pqxx::internal_error{"No outcome reached on perform()."};
}
} // namespace pqxx
//@}

#include "pqxx/compiler-internal-post.hxx"
#endif
