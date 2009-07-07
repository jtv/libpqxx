namespace pqxx
{
class tablewriter;

namespace internal
{
class PQXX_PRIVATE transaction_tablewriter_gate
{
  friend class pqxx::tablewriter;

  transaction_tablewriter_gate(transaction_base &home) : m_home(home) {}

  void BeginCopyWrite(
	const PGSTD::string &table,
	const PGSTD::string &columns = PGSTD::string())
  {
    m_home.BeginCopyWrite(table, columns);
  }

  void WriteCopyLine(const PGSTD::string &line) { m_home.WriteCopyLine(line); }

  void EndCopyWrite() { m_home.EndCopyWrite(); }

  transaction_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
