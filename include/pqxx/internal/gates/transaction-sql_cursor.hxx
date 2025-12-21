#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
// XXX: Use this or lose this!
class PQXX_PRIVATE transaction_sql_cursor final : callgate<transaction_base>
{
  friend class pqxx::internal::sql_cursor;
  constexpr transaction_sql_cursor(reference x) noexcept : super(x) {}
};
} // namespace pqxx::internal::gate
