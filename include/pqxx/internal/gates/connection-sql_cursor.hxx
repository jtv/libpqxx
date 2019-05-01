#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
class sql_cursor;

namespace gate
{
class PQXX_PRIVATE connection_sql_cursor : callgate<connection_base>
{
  friend class pqxx::internal::sql_cursor;

  connection_sql_cursor(reference x) : super(x) {}

  result exec(const char query[]) { return home().exec(query); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
