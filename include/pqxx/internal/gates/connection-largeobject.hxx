#include <pqxx/internal/callgate.hxx>
#include <pqxx/internal/libpq-forward.hxx>

namespace pqxx
{
class largeobject;

namespace internal
{
namespace gate
{
class PQXX_PRIVATE connection_largeobject : callgate<connection_base>
{
  friend class pqxx::largeobject;

  connection_largeobject(reference x) : super(x) {}

  pq::PGconn *raw_connection() const { return home().raw_connection(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
