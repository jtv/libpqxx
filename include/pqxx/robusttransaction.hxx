/* Definition of the pqxx::robusttransaction class.
 *
 * pqxx::robusttransaction is a slower but safer transaction class.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/robusttransaction instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ROBUSTTRANSACTION
#define PQXX_H_ROBUSTTRANSACTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/dbtransaction.hxx"

namespace pqxx::internal
{
/// Helper base class for the @c robusttransaction class template.
class PQXX_LIBEXPORT PQXX_NOVTABLE basic_robusttransaction
        : public dbtransaction
{
public:
  virtual ~basic_robusttransaction() override = 0;

protected:
  basic_robusttransaction(connection &c, char const begin_command[]);

private:
  using IDType = unsigned long;

  std::string m_conn_string;
  std::string m_xid;
  int m_backendpid = -1;

  virtual void do_commit() override;
  virtual void do_abort() override;
};
} // namespace pqxx::internal


namespace pqxx
{
/**
 * @ingroup transaction
 *
 * @{
 */

/// Slightly slower, better-fortified version of transaction
/** Requires PostgreSQL 10 or better.
 *
 * robusttransaction is similar to transaction, but spends more time and effort
 * to deal with the hopefully rare case that the connection to the backend is
 * lost just while it's trying to commit.  In such cases, the client does not
 * know whether the backend (on the other side of the broken connection)
 * managed to commit the transaction.
 *
 * Whe this happens, robusttransaction tries to reconnect to the database and
 * figure out what happened.
 *
 * This service level was made optional since you may not want to pay the
 * overhead where it is not necessary.  Certainly the use of this class makes
 * no sense for local connections, or for transactions that read the database
 * but never modify it, or for noncritical database manipulations.
 *
 * Besides being slower, it's also more complex.  Which means that in practice
 * a robusttransaction could actually fail more instead of less often than a
 * normal transaction.  What robusttransaction tries to achieve is to give you
 * certainty, not just be more successful per se.
 */
template<isolation_level ISOLATION = read_committed>
class robusttransaction final : public internal::basic_robusttransaction
{
public:
  /** Create robusttransaction of given name.
   * @param c Connection inside which this robusttransaction should live.
   * @param name optional human-readable name for this transaction.
   */
  explicit robusttransaction(
    connection &c, std::string_view name = std::string_view{}) :
          namedclass{"robusttransaction", name},
          internal::basic_robusttransaction{
            c,
            pqxx::internal::begin_cmd<ISOLATION, write_policy::read_write>.c_str()}
  {}

  virtual ~robusttransaction() noexcept override { close(); }
};

/**
 * @}
 */
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
