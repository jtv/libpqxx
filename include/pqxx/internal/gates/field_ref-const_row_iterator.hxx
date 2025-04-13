#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class field_ref;
} // namespace pqxx


namespace pqxx::internal::gate
{
class PQXX_PRIVATE field_ref_const_row_iterator : callgate<field_ref>
{
  friend class pqxx::const_row_iterator;

  field_ref_const_row_iterator(reference x) : super(x) {}

  void offset(row_difference_type d) { home().offset(d); }
};
} // namespace pqxx::internal::gate
