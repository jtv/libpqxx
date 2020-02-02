/* Definition of the pqxx::transaction class.
 * pqxx::transaction represents a standard database transaction.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/transaction instead.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_TRANSACTION
#define PQXX_H_TRANSACTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/dbtransaction.hxx"

namespace pqxx::internal
{
/// Helper base class for the @c transaction class template.
class PQXX_LIBEXPORT basic_transaction : public dbtransaction
{
protected:
  basic_transaction(connection &c, char const begin_command[]);

private:
  virtual void do_commit() override;
  virtual void do_abort() override;
};
} // namespace pqxx::internal


namespace pqxx
{
/**
 * @ingroup transaction
 */
//@{

/// Standard back-end transaction, templatized on isolation level
/** This is the type you'll normally want to use to represent a transaction on
 * the database.
 *
 * Usage example: double all wages.
 *
 * @code
 * extern connection C;
 * work T(C);
 * try
 * {
 *   T.exec0("UPDATE employees SET wage=wage*2");
 *   T.commit();	// NOTE: do this inside try block
 * }
 * catch (exception const &e)
 * {
 *   cerr << e.what() << endl;
 *   T.abort();		// Usually not needed; same happens when T's life ends.
 * }
 * @endcode
 */
template<
  isolation_level ISOLATION = isolation_level::read_committed,
  write_policy READWRITE = write_policy::read_write>
class transaction final : public internal::basic_transaction
{
public:
  /// Create a transaction.
  /**
   * @param c Connection for this transaction to operate on.
   * @param tname Optional name for transaction.  Must begin with a letter and
   * may contain letters and digits only.
   */
  explicit transaction(connection &c, std::string const &tname) :
          namedclass{"transaction", tname},
          internal::basic_transaction(
            c, internal::begin_cmd<ISOLATION, READWRITE>.c_str())
  {}

  explicit transaction(connection &c) : transaction(c, "") {}

  virtual ~transaction() noexcept override { close(); }
};


/// The default transaction type.
using work = transaction<>;

/// Read-only transaction.
using read_transaction =
  transaction<isolation_level::read_committed, write_policy::read_only>;

//@}
} // namespace pqxx

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
