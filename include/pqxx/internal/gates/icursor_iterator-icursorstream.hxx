#ifndef PQXX_INTERNAL_GATES_ICURSOR_ITERATOR_ICURSORSTREAM_HXX
#define PQXX_INTERNAL_GATES_ICURSOR_ITERATOR_ICURSORSTREAM_HXX

#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE icursor_iterator_icursorstream final
        : callgate<icursor_iterator>
{
  friend class pqxx::icursorstream;

  explicit constexpr icursor_iterator_icursorstream(reference x) noexcept :
          super(x)
  {}

  [[nodiscard]] icursor_iterator::difference_type pos() const noexcept
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
#endif
