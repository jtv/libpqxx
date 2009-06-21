namespace pqxx
{
class dbtransaction;

namespace internal
{
class PQXX_PRIVATE connection_dbtransaction_gate
{
  friend class pqxx::dbtransaction;

  connection_dbtransaction_gate(connection_base &home) : m_home(home) {}

  int get_reactivation_avoidance_count() const throw ()
	{ return m_home.m_reactivation_avoidance.get(); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
