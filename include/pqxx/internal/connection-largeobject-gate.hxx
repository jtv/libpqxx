#include <pqxx/libpq-forward.hxx>

namespace pqxx
{
class largeobject;

namespace internal
{
class PQXX_PRIVATE connection_largeobject_gate
{
  friend class pqxx::largeobject;

  connection_largeobject_gate(connection_base &home) : m_home(home) {}

  pq::PGconn *RawConnection() const { return m_home.RawConnection(); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
