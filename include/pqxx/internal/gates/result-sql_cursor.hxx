#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_sql_cursor final : callgate<result const>
{
  friend class pqxx::internal::sql_cursor;

  constexpr result_sql_cursor(reference x) noexcept : super(x) {}

  [[nodiscard]] char const *cmd_status() const noexcept
  {
    return home().cmd_status();
  }
};
} // namespace pqxx::internal::gate
