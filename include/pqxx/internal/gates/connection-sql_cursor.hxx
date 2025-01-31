#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal
{
class sql_cursor;
}


namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_sql_cursor : callgate<connection>
{
  friend class pqxx::internal::sql_cursor;

  connection_sql_cursor(reference x) : super(x) {}

  result exec(char const query[], PQXX_LOC loc)
  {
    return home().exec(query, loc);
  }
};
} // namespace pqxx::internal::gate
