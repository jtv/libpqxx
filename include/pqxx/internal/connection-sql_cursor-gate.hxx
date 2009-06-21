namespace pqxx
{
namespace internal
{
class sql_cursor;

class PQXX_PRIVATE connection_sql_cursor_gate
{
  friend class pqxx::internal::sql_cursor;

  connection_sql_cursor_gate(connection_base &home) : m_home(home) {}

  result Exec(const char query[], int retries)
	{ return m_home.Exec(query, retries); }

  void add_reactivation_avoidance_count(int n)
	{ m_home.add_reactivation_avoidance_count(n); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
