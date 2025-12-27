#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_connection final : callgate<result const>
{
  friend class pqxx::connection;

  constexpr result_connection(reference x) noexcept : super(x) {}

  [[nodiscard]] operator bool() const noexcept { return bool(home()); }
  [[nodiscard]] bool operator!() const noexcept { return not home(); }
};
} // namespace pqxx::internal::gate
