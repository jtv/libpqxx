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
  std::size_t operator()(std::string const &x) const noexcept
  {
    return static_cast<uint8_t>(x[0]);
  }
};


// TODO: Is there a simple, lightweight, constexpr alternative?
std::unordered_map<std::string, tx_stat, initial_hash> const statuses{
  {"committed", tx_committed},
  {"aborted", tx_aborted},
  {"in progress", tx_in_progress},
};


tx_stat query_status(std::string const &xid, std::string const &conn_str)
{
  static std::string const name{"robusttxck"};
  auto const query{"SELECT txid_status(" + xid + ")"};
  pqxx::connection c{conn_str};
  pqxx::nontransaction w{c, name};
  auto const status_text{w.query_value<std::string>(query)};
  if (status_text.empty())
    throw pqxx::internal_error{"Transaction status string is empty."};
  auto const here{statuses.find(status_text)};
  if (here == statuses.end())
    throw pqxx::internal_error{"Unknown transaction status: " + status_text};
  return here->second;
}
} // namespace


pqxx::internal::basic_robusttransaction::basic_robusttransaction(
  connection &c, char const begin_command[]) :
        namedclass{"robusttransaction"},
        dbtransaction(c),
        m_conn_string{c.connection_string()}
{
  m_backendpid = c.backendpid();
  direct_exec(begin_command);
  direct_exec("SELECT txid_current()")[0][0].to(m_xid);
}


pqxx::internal::basic_robusttransaction::~basic_robusttransaction() = default;


void pqxx::internal::basic_robusttransaction::do_commit()
{
  // Check constraints before sending the COMMIT to the database, so as to
  // minimise our in-doubt window.
  try
  {
    direct_exec("SET CONSTRAINTS ALL IMMEDIATE");
  }
  catch (std::exception const &)
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
  catch (broken_connection const &)
  {
    // Oops, lost connection at the crucial moment.  Fall through to in-doubt
    // handling below.
  }
  catch (std::exception const &)
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

  auto const delay{std::chrono::milliseconds(300)};
  int const max_attempts{500};
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
    catch (pqxx::broken_connection const &)
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
