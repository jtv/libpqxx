namespace pqxx
{
class connection_base;

namespace internal
{
class PQXX_PRIVATE connection_transaction_gate
{
  friend class pqxx::transaction_base;

  connection_transaction_gate(connection_base &home) : m_home(home) {}

  result Exec(const char query[], int retries)
	{ return m_home.Exec(query, retries); }
  void RegisterTransaction(transaction_base *t)
	{ m_home.RegisterTransaction(t); }
  void UnregisterTransaction(transaction_base *t) throw ()
	{ m_home.UnregisterTransaction(t); }

  bool ReadCopyLine(PGSTD::string &line)
	{ return m_home.ReadCopyLine(line); }
  void WriteCopyLine(const PGSTD::string &line)
	{ m_home.WriteCopyLine(line); }
  void EndCopyWrite() { m_home.EndCopyWrite(); }

  PGSTD::string RawGetVar(const PGSTD::string &var)
	{ return m_home.RawGetVar(var); }
  void RawSetVar(const PGSTD::string &var, const PGSTD::string &value)
	{ m_home.RawSetVar(var, value); }
  void AddVariables(const PGSTD::map<PGSTD::string, PGSTD::string> &vars)
	{ m_home.AddVariables(vars); }

  result prepared_exec(const PGSTD::string &statement,
	const char *const params[],
	const int paramlengths[],
	int nparams)
  {
    return m_home.prepared_exec(statement, params, paramlengths, nparams);
  }

  bool prepared_exists(const PGSTD::string &statement) const
	{ return m_home.prepared_exists(statement); }

  void take_reactivation_avoidance(int counter)
	{ m_home.m_reactivation_avoidance.add(counter); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
