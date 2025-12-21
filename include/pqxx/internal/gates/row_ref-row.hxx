#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE row_ref_row final : callgate<row_ref>
{
  friend class pqxx::row;

  constexpr row_ref_row(reference x) noexcept : super(x) {}

  template<typename Tuple, std::size_t... indexes>
  void
  extract_fields(Tuple &t, std::index_sequence<indexes...> seq, ctx c) const
  {
    return home().extract_fields<Tuple, indexes...>(t, seq, c);
  }
};
} // namespace pqxx::internal::gate
