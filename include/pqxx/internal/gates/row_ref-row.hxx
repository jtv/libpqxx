#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE row_ref_row : callgate<row_ref>
{
  friend class pqxx::row;

  row_ref_row(reference x) noexcept : super(x) {}

  template<typename Tuple, std::size_t... indexes>
  void
  extract_fields(Tuple &t, std::index_sequence<indexes...> seq, sl loc) const
  {
    return home().extract_fields<Tuple, indexes...>(t, seq, loc);
  }
};
} // namespace pqxx::internal::gate
