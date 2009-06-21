namespace pqxx
{
class connection_base;
class pipeline;

namespace internal
{
class PQXX_PRIVATE result_creation_gate
{
  friend class pqxx::connection_base;
  friend class pqxx::pipeline;

  result_creation_gate(const result &home) : m_home(home) {}

  static result create(
	internal::pq::PGresult *rhs,
	int protocol,
	const PGSTD::string &query,
	int encoding_code)
  {
    return result(rhs, protocol, query, encoding_code);
  }

  void CheckStatus() const { return m_home.CheckStatus(); }

  const result &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
