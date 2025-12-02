#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class row_ref;
} // namespace pqxx


namespace pqxx::internal::gate
{
class PQXX_PRIVATE row_ref_result : callgate<row_ref const>
{
  friend class pqxx::result;

  row_ref_result(reference x) noexcept : super(x) {}

  template<typename TUPLE> TUPLE as_tuple(sl loc) const
  {
    return home().as_tuple<TUPLE>(loc);
  }
};
} // namespace pqxx::internal::gate
