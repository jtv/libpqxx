/** Result loops.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_RESULT_ITER
#define PQXX_H_RESULT_ITER

#include <memory>

#include "pqxx/internal/gates/row_ref-result.hxx"
#include "pqxx/strconv.hxx"

namespace pqxx
{
class result;
} // namespace pqxx


namespace pqxx::internal
{
// TODO: Replace with generator?
/// Iterator for looped unpacking of a result.
template<typename... TYPE> class result_iter final
{
public:
  using value_type = std::tuple<TYPE...>;

  /// Construct an "end" iterator.
  result_iter() = default;

  explicit result_iter(result const &home, sl loc = sl::current()) :
          m_home{&home}, m_size{std::size(home)}
  {
    if (not std::empty(home))
      read(loc);
  }
  result_iter(result_iter const &) = default;

  result_iter &operator++()
  {
    PQXX_ASSUME(m_home != nullptr);
    PQXX_ASSUME(m_index <= m_size);
    // TODO: Would be nice to get at least the result's creation location.
    sl loc{sl::current()};
    ++m_index;
    if (m_index >= m_size)
      m_home = nullptr;
    else if (m_home != nullptr) [[likely]]
      read(loc);
    return *this;
  }

  /// Comparison only works for comparing to end().
  bool operator==(result_iter const &rhs) const noexcept
  {
    return m_home == rhs.m_home;
  }
  bool operator!=(result_iter const &rhs) const noexcept
  {
    return not(*this == rhs);
  }

  value_type const &operator*() const noexcept { return m_value; }

private:
  void read(sl loc) { (*m_home)[m_index].convert(m_value, loc); }

  result const *m_home{nullptr};
  result::size_type m_index{0};
  result::size_type m_size;
  value_type m_value;
};


/// Iterator for implementing @ref pqxx::result::iter().
template<typename... TYPE> class result_iteration final
{
public:
  using iterator = result_iter<TYPE...>;

  explicit result_iteration(result const &home) : m_home{home}
  {
    m_home.expect_columns(sizeof...(TYPE));
  }

  iterator begin() const
  {
    if (std::size(m_home) == 0)
      return end();
    else
      return iterator{m_home};
  }
  iterator end() const { return {}; }

private:
  pqxx::result const m_home;
};
} // namespace pqxx::internal


template<typename... TYPE> inline auto pqxx::result::iter() const
{
  return pqxx::internal::result_iteration<TYPE...>{*this};
}


template<typename CALLABLE>
inline void pqxx::result::for_each(CALLABLE &&func, sl loc) const
{
  using args_tuple = internal::args_t<decltype(func)>;
  constexpr auto sz{std::tuple_size_v<args_tuple>};
  static_assert(
    sz > 0,
    "Callback for for_each must take parameters, one for each column in the "
    "result.");

  auto const cols{this->columns()};
  if (sz != cols)
    throw usage_error{
      std::format(
        "Callback to for_each takes {} parameter(s), but result set has {} "
        "field(s).",
        sz, cols),
      loc};

  using pass_tuple = pqxx::internal::strip_types_t<args_tuple>;
  for (auto const r : *this)
    std::apply(
      func, pqxx::internal::gate::row_ref_result{r}.as_tuple<pass_tuple>(loc));
}
#endif
