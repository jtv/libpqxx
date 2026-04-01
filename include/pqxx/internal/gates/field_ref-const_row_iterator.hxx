#ifndef PQXX_INTERNAL_GATES_FIELD_REF_CONST_ROW_ITERATOR_HXX
#define PQXX_INTERNAL_GATES_FIELD_REF_CONST_ROW_ITERATOR_HXX

#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class field_ref;
} // namespace pqxx


namespace pqxx::internal::gate
{
class PQXX_PRIVATE field_ref_const_row_iterator final : callgate<field_ref>
{
  friend class pqxx::const_row_iterator;

  explicit constexpr field_ref_const_row_iterator(reference x) noexcept :
          super(x)
  {}

  void offset(row_difference_type d) { home().offset(d); }
};
} // namespace pqxx::internal::gate
#endif
