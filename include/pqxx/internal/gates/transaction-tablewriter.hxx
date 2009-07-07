#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE transaction_tablewriter : callgate<transaction_base>
{
  friend class pqxx::tablewriter;

  transaction_tablewriter(reference x) : super(x) {}

  void BeginCopyWrite(
	const PGSTD::string &table,
	const PGSTD::string &columns = PGSTD::string())
	{ home().BeginCopyWrite(table, columns); }

  void WriteCopyLine(const PGSTD::string &line) { home().WriteCopyLine(line); }

  void EndCopyWrite() { home().EndCopyWrite(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
