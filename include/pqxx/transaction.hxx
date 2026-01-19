/* Definition of the pqxx::transaction class.
 * pqxx::transaction represents a standard database transaction.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transaction instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_TRANSACTION
#define PQXX_H_TRANSACTION

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include "pqxx/dbtransaction.hxx"

namespace pqxx::internal
{
/// Helper base class for the @ref pqxx::transaction class template.
class PQXX_LIBEXPORT basic_transaction : public dbtransaction
{
protected:
  basic_transaction(
    connection &cx, zview begin_command, std::string_view tname,
    sl = sl::current());
  basic_transaction(
    connection &cx, zview begin_command, std::string &&tname,
    sl = sl::current());
  basic_transaction(connection &cx, zview begin_command, sl = sl::current());

  ~basic_transaction() noexcept override = 0;

private:
  void do_commit(sl) override;
};
} // namespace pqxx::internal


namespace pqxx
{
/// transaction Standard back-end transaction, templatised on isolation level.
/** @ingroup transactions
 * This is the type you'll normally want to use to represent a transaction on
 * the database.
 *
 * @tparam ISOLATION The transaction's "isolation level."  This is a standard
 * SQL concept.  The default in PostgreSQL (and in libpqxx) is READ COMMITTED.
 * @tparam READWRITE Whether this transaction is allowed to make changes to the
 * database.
 *
 * Usage example: double all wages.
 *
 * ```cxx
 * extern pqxx::connection cx;
 * pqxx::work tx(cx);
 * try
 * {
 *   tx.exec("UPDATE employees SET wage=wage*2").no_rows();
 *   tx.commit();  // NOTE: do this inside try block
 * }
 * catch (std::exception const &e)
 * {
 *   std::cerr << e.what() << '\n';
 *   tx.abort();  // Usually not needed; same happens when tx's life ends.
 * }
 * ```
 */
template<
  isolation_level ISOLATION = isolation_level::read_committed,
  write_policy READWRITE = write_policy::read_write>
class transaction final : public internal::basic_transaction
{
public:
  /// Begin a transaction.
  /**
   * @param cx Connection for this transaction to operate on.
   * @param tname Optional name for transaction.  Must begin with a letter and
   * may contain letters and digits only.
   */
  transaction(connection &cx, std::string_view tname, sl loc = sl::current()) :
          internal::basic_transaction{
            cx, internal::begin_cmd<ISOLATION, READWRITE>, tname, loc}
  {}

  /// Begin a transaction.
  /**
   * @param cx Connection for this transaction to operate on.
   * may contain letters and digits only.
   */
  explicit transaction(connection &cx, sl loc = sl::current()) :
          internal::basic_transaction{
            cx, internal::begin_cmd<ISOLATION, READWRITE>, loc}
  {}

  ~transaction() noexcept override { close(sl::current()); }
};


/// The default transaction type.
/** @ingroup transactions
 */
using work =
  transaction<isolation_level::read_committed, write_policy::read_write>;


/// Read-only transaction.
/** @ingroup transactions
 */
using read_transaction =
  transaction<isolation_level::read_committed, write_policy::read_only>;

} // namespace pqxx
#endif
