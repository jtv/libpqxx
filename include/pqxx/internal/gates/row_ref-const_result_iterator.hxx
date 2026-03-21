#ifndef PQXX_INTERNAL_GATES_ROW_REF_CONST_RESULT_ITERATOR_HXX
#define PQXX_INTERNAL_GATES_ROW_REF_CONST_RESULT_ITERATOR_HXX

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

  explicit constexpr row_ref_const_result_iterator(reference x) noexcept :
          super(x)
  {}

  void offset(result_difference_type d) { home().offset(d); }
};
} // namespace pqxx::internal::gate
#endif
