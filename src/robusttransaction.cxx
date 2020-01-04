/** Implementation of the pqxx::robusttransaction class.
 *
 * pqxx::robusttransaction is a slower but safer transaction class.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <chrono>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <unordered_map>

#include "pqxx/connection"
#include "pqxx/nontransaction"
#include "pqxx/result"
#include "pqxx/robusttransaction"


namespace
{
/// Statuses in which we may find our transaction.
/** There's also "in the future," but it manifests as an error, not as an
 * actual status.
 */
enum tx_stat
{
  tx_unknown,
  tx_committed,
  tx_aborted,
  tx_in_progress,
};


/* Super-simple string hash function: just use the initial byte.
 *
 * Happens to be a perfect hash for this case, and should be cheap to compute.
 */
struct initial_hash
{
  size_t operator()(const std::string &x) const noexcept
  {
    return static_cast<uint8_t>(x[0]);
  }
};


// TODO: Is there a simple, lightweight, constexpr alternative?
const std::unordered_map<std::string, tx_stat, initial_hash> statuses{
  {"committed", tx_committed},
  {"aborted", tx_aborted},
  {"in progress", tx_in_progress},
};


tx_stat query_status(const std::string &xid, const std::string conn_str)
{
  const std::string name{"robustx-check-status"};
  const std::string query{"SELECT txid_status(" + xid + ")"};
  pqxx::connection c{conn_str};
  pqxx::nontransaction w{c, name};
  const auto row{w.exec1(query, name)};
  const auto status_text{row[0].as<std::string>()};
  if (status_text.empty())
    throw pqxx::internal_error{"Transaction status string is empty."};
  const auto here{statuses.find(status_text)};
  if (here == statuses.end())
    throw pqxx::internal_error{"Unknown transaction status: " + status_text};
  return here->second;
}
} // namespace


pqxx::internal::basic_robusttransaction::basic_robusttransaction(
  connection &C, const char begin_command[]) :
        namedclass{"robusttransaction"},
        dbtransaction(C),
        m_conn_string{C.connection_string()}
{
  m_backendpid = C.backendpid();
  direct_exec(begin_command);
  direct_exec("SELECT txid_current()")[0][0].to(m_xid);
}


pqxx::internal::basic_robusttransaction::~basic_robusttransaction() {}


void pqxx::internal::basic_robusttransaction::do_commit()
{
  // Check constraints before sending the COMMIT to the database, so as to
  // minimise our in-doubt window.
  try
  {
    direct_exec("SET CONSTRAINTS ALL IMMEDIATE");
  }
  catch (const std::exception &)
  {
    do_abort();
    throw;
  }

  // Here comes the in-doubt window.  If we lose our connection here, we'll be
  // left clueless as to what happened on the backend.  It may have received
  // the commit command and completed the transaction, and ended up with a
  // success it could not report back to us.  Or it may have noticed the broken
  // connection and aborted the transaction.  It may even still be executing
  // the commit, only to fail later.
  //
  // All this uncertainty requires some special handling, and that s what makes
  // robusttransaction what it is.
  try
  {
    direct_exec("COMMIT");

    // If we make it here, great.  Normal, successful commit.
    return;
  }
  catch (const broken_connection &)
  {
    // Oops, lost connection at the crucial moment.  Fall through to in-doubt
    // handling below.
  }
  catch (const std::exception &)
  {
    if (conn().is_open())
    {
      // Commit failed, for some other reason.
      do_abort();
      throw;
    }
    // Otherwise, fall through to in-doubt handling.
  }

  // If we get here, we're in doubt.  Figure out what happened.

  // TODO: Make delay and attempts configurable.
  const auto delay{std::chrono::milliseconds(300)};
  const int max_attempts{500};
  static_assert(max_attempts > 0);

  tx_stat stat;
  for (int attempts{0}; attempts < max_attempts;
       ++attempts, std::this_thread::sleep_for(delay))
  {
    stat = tx_unknown;
    try
    {
      stat = query_status(m_xid, m_conn_string);
    }
    catch (const pqxx::broken_connection &)
    {
      // Swallow the error.  Pause and retry.
    }
    switch (stat)
    {
    case tx_unknown:
      // We were unable to reconnect and query transaction status.
      // Stay in it for another attempt.
      return;
    case tx_committed:
      // Success!  We're done.
      return;
    case tx_aborted:
      // Aborted.  We're done.
      do_abort();
      return;
    case tx_in_progress:
      // The transaction is still running.  Stick around until we know what
      // transpires.
      break;
    }
  }

  // Okay, this has taken too long.  Give up, report in-doubt state.
  throw in_doubt_error{
    "Transaction " + name() + " (with transaction ID " + m_xid +
    ") "
    "lost connection while committing.  It's impossible to tell whether "
    "it committed, or aborted, or is still running.  "
    "Attempts to find out its outcome have failed.  "
    "The backend process on the server had process ID " +
    to_string(m_backendpid) +
    ".  "
    "You may be able to check what happened to that process."};
}


void pqxx::internal::basic_robusttransaction::do_abort()
{
  direct_exec("ROLLBACK");
}
