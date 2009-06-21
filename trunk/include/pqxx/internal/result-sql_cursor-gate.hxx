namespace pqxx
{
namespace internal
{
class sql_cursor;

class PQXX_PRIVATE result_sql_cursor_gate
{
  friend class pqxx::internal::sql_cursor;

  result_sql_cursor_gate(const result &home) : m_home(home) {}

  const char *CmdStatus() const throw () { return m_home.CmdStatus(); }

  const result &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
