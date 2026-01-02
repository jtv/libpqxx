#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal
{
class sql_cursor;
}


namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_sql_cursor final : callgate<connection>
{
  friend class pqxx::internal::sql_cursor;

  constexpr connection_sql_cursor(reference x) noexcept : super(x) {}

  PQXX_ZARGS result exec(char const query[], sl loc)
  {
    return home().exec(query, loc);
  }
};
} // namespace pqxx::internal::gate
