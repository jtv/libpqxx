#ifndef PQXX_INTERNAL_GATES_RESULT_CONNECTION_HXX
#define PQXX_INTERNAL_GATES_RESULT_CONNECTION_HXX

#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_connection final : callgate<result const>
{
  friend class pqxx::connection;

  explicit constexpr result_connection(reference x) noexcept : super(x) {}

  [[nodiscard]] explicit operator bool() const noexcept
  {
    return bool(home());
  }
  [[nodiscard]] bool operator!() const noexcept { return not home(); }
};
} // namespace pqxx::internal::gate
#endif
