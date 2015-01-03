#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE transaction_transactionfocus : callgate<transaction_base>
{
  friend class pqxx::internal::transactionfocus;

  transaction_transactionfocus(reference x) : super(x) {}

  void RegisterFocus(transactionfocus *focus) { home().RegisterFocus(focus); }
  void UnregisterFocus(transactionfocus *focus) PQXX_NOEXCEPT
	{ home().UnregisterFocus(focus); }
  void RegisterPendingError(const std::string &error)
	{ home().RegisterPendingError(error); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
