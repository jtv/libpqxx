#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE transaction_tablereader : callgate<transaction_base>
{
  friend class pqxx::tablereader;

  transaction_tablereader(reference x) : super(x) {}

  void BeginCopyRead(const PGSTD::string &table, const PGSTD::string &columns)
	{ home().BeginCopyRead(table, columns); }

  bool ReadCopyLine(PGSTD::string &line) { return home().ReadCopyLine(line); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
