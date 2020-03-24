/** Base class for things which lay a special claim on a transaction.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
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
class sql_cursor;

class PQXX_LIBEXPORT transactionfocus : public virtual namedclass
{
public:
  explicit transactionfocus(transaction_base &t) :
          namedclass{"transactionfocus"}, m_trans{t}
  {}

  transactionfocus() = delete;
  transactionfocus(transactionfocus const &) = delete;
  transactionfocus &operator=(transactionfocus const &) = delete;

protected:
  void register_me();
  void unregister_me() noexcept;
  void reg_pending_error(std::string const &) noexcept;
  bool registered() const noexcept { return m_registered; }

  transaction_base &m_trans;

private:
  bool m_registered = false;
};
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
