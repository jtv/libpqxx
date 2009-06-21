namespace pqxx
{
namespace prepare
{
class invocation;
} // namespace pqxx::prepare

namespace internal
{
class PQXX_PRIVATE connection_prepare_invocation_gate
{
  friend class pqxx::prepare::invocation;

  connection_prepare_invocation_gate(connection_base &home) : m_home(home) {}

  result prepared_exec(
	const PGSTD::string &statement,
	const char *const params[],
	const int paramlengths[],
	int nparams)
  {
    return m_home.prepared_exec(statement, params, paramlengths, nparams);
  }

  bool prepared_exists(const PGSTD::string &statement) const
	{ return m_home.prepared_exists(statement); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
