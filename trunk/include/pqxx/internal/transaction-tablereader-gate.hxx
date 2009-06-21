namespace pqxx
{
class tablereader;

namespace internal
{
class PQXX_PRIVATE transaction_tablereader_gate
{
  friend class pqxx::tablereader;

  transaction_tablereader_gate(transaction_base &home) : m_home(home) {}

  void BeginCopyRead(const PGSTD::string &table, const PGSTD::string &columns)
	{ m_home.BeginCopyRead(table, columns); }

  bool ReadCopyLine(PGSTD::string &line) { return m_home.ReadCopyLine(line); }

  transaction_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
