/** Base class for things which lay a special claim on a transaction.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_TRANSACTION_FOCUS
#define PQXX_H_TRANSACTION_FOCUS

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

#include "pqxx/util.hxx"

namespace pqxx::internal
{
/// Base class for things that monopolise a transaction.
class PQXX_LIBEXPORT transactionfocus
{
public:
  transactionfocus(
    transaction_base &t, std::string_view cname, std::string_view oname) :
          m_trans{t}, m_classname{cname}, m_name{oname}
  {}

  transactionfocus(
    transaction_base &t, std::string_view cname, std::string &&oname) :
          m_trans{t}, m_classname{cname}, m_name{std::move(oname)}
  {}

  transactionfocus(transaction_base &t, std::string_view cname) :
          m_trans{t}, m_classname{cname}
  {}

  transactionfocus() = delete;
  transactionfocus(transactionfocus const &) = delete;
  transactionfocus &operator=(transactionfocus const &) = delete;

  /// Class name, for human consumption.
  [[nodiscard]] std::string_view classname() const noexcept
  {
    return m_classname;
  }

  /// Name for this object, if the caller passed one; empty string otherwise.
  [[nodiscard]] std::string_view name() const noexcept { return m_name; }

  [[nodiscard]] std::string description() const
  {
    return describe_object(m_classname, m_name);
  }

  /// Can't move a transactionfocus.
  /** Moving the transactionfocus would break the transaction's reference back
   * to the object.
   */
  transactionfocus(transactionfocus &&) = delete;

  /// Can't move a transactionfocus.
  /** Moving the transactionfocus would break the transaction's reference back
   * to the object.
   */
  transactionfocus &operator=(transactionfocus &&) = delete;

protected:
  void register_me();
  void unregister_me() noexcept;
  void reg_pending_error(std::string const &) noexcept;
  bool registered() const noexcept { return m_registered; }

  transaction_base &m_trans;

private:
  bool m_registered = false;
  std::string_view m_classname;
  std::string m_name;
};
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
