namespace pqxx
{
namespace internal
{
class PQXX_PRIVATE connection_prepare_declaration_gate
{
  friend class prepare::declaration;

  connection_prepare_declaration_gate(connection_base &home) : m_home(home) {}

  void prepare_param_declare(
	const PGSTD::string &statement,
	const PGSTD::string &sqltype,
	prepare::param_treatment treatment)
  {
    m_home.prepare_param_declare(statement, sqltype, treatment);
  }

  void prepare_param_declare_varargs(
	const PGSTD::string &statement, prepare::param_treatment treatment)
  {
    m_home.prepare_param_declare_varargs(statement, treatment);
  }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
