namespace pqxx
{
class subtransaction;

namespace internal
{
class PQXX_PRIVATE transaction_subtransaction_gate
{
  friend class pqxx::subtransaction;

  transaction_subtransaction_gate(transaction_base &home) : m_home(home) {}

  void add_reactivation_avoidance_count(int n)
  {
    m_home.m_reactivation_avoidance.add(n);
  }

  transaction_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
