#ifndef INTERNAL_GATES_ERRORHANDLER_CONNECTION_HXX
#define INTERNAL_GATES_ERRORHANDLER_CONNECTION_HXX

#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE errorhandler_connection final : callgate<errorhandler>
{
  friend class pqxx::connection;

  explicit constexpr errorhandler_connection(reference x) noexcept : super(x)
  {}

  void unregister() noexcept { home().unregister(); }
};
} // namespace pqxx::internal::gate
#endif
