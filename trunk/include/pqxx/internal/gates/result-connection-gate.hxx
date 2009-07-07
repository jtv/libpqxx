namespace pqxx
{
class connection_base;

namespace internal
{
class PQXX_PRIVATE result_connection_gate
{
  friend class pqxx::connection_base;

  result_connection_gate(const result &home) : m_home(home) {}

  operator bool() const { return bool(m_home); }
  bool operator!() const { return !m_home; }

  const result &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
