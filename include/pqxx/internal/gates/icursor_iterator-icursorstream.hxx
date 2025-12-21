#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE icursor_iterator_icursorstream final
        : callgate<icursor_iterator>
{
  friend class pqxx::icursorstream;

  constexpr icursor_iterator_icursorstream(reference x) noexcept : super(x) {}

  icursor_iterator::difference_type pos() const noexcept
  {
    return home().pos();
  }

  [[nodiscard]] icursor_iterator *get_prev() const noexcept
  {
    return home().m_prev;
  }
  void set_prev(icursor_iterator *i) noexcept { home().m_prev = i; }

  [[nodiscard]] icursor_iterator *get_next() const noexcept
  {
    return home().m_next;
  }
  void set_next(icursor_iterator *i) { home().m_next = i; }

  void fill(result const &r) { home().fill(r); }
};
} // namespace pqxx::internal::gate
