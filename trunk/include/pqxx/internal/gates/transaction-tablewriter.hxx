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
	const std::string &table,
	const std::string &columns = std::string())
	{ home().BeginCopyWrite(table, columns); }

  void WriteCopyLine(const std::string &line) { home().WriteCopyLine(line); }

  void EndCopyWrite() { home().EndCopyWrite(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
