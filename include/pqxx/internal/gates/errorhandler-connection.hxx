#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE errorhandler_connection : callgate<errorhandler>
{
  friend class pqxx::connection;

  errorhandler_connection(reference x) : super(x) {}

  void unregister() noexcept { home().unregister(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
