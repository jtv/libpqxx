#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class row_ref;
} // namespace pqxx


namespace pqxx::internal::gate
{
class PQXX_PRIVATE row_ref_const_result_iterator final : callgate<row_ref>
{
  friend class pqxx::const_result_iterator;

  constexpr row_ref_const_result_iterator(reference x) noexcept : super(x) {}

  void offset(result_difference_type d) { home().offset(d); }
};
} // namespace pqxx::internal::gate
