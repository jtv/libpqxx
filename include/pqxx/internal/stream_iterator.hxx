/** Stream iterators.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_STREAM_ITERATOR
#define PQXX_H_STREAM_ITERATOR

#include "pqxx/compiler-public.hxx"
#include "pqxx/internal/compiler-internal-pre.hxx"

namespace pqxx
{
class stream_from;
}


namespace pqxx::internal
{
// TODO: Replace with C++20 generator.
/// Input iterator for stream_from.
template<typename... TYPE> class stream_input_iterator
{
public:
  using value_type = std::tuple<TYPE...>;

  /// Construct an "end" iterator.
  stream_input_iterator() = default;

  explicit stream_input_iterator(stream_from &home) : m_home(&home)
  {
    advance();
  }
  stream_input_iterator(stream_input_iterator const &) = default;

  stream_input_iterator &operator++()
  {
    advance();
    return *this;
  }
  stream_input_iterator operator++(int)
  {
    stream_input_iterator next{*this};
    return ++next;
  }

  const value_type &operator*() const { return m_value; }
  const value_type *operator->() const { return &m_value; }

  bool operator==(stream_input_iterator const &rhs) const
  {
    return m_home == rhs.m_home and
           (m_home == nullptr or m_offset == rhs.m_offset);
  }
  bool operator!=(stream_input_iterator const &rhs) const
  {
    return not(*this == rhs);
  }

private:
  void advance()
  {
    if (m_home == nullptr)
      throw usage_error{"Moving stream_from iterator beyond end()."};
    ++m_offset;
    if (not((*m_home) >> m_value))
      m_home = nullptr;
  }

  stream_from *m_home = nullptr;
  size_t m_offset = 0;
  value_type m_value;
};


template<typename... TYPE> class stream_input_iteration
{
public:
  using iterator = stream_input_iterator<TYPE...>;
  explicit stream_input_iteration(stream_from &home) : m_home(home) {}
  iterator begin() const { return iterator{m_home}; }
  iterator end() const { return iterator{}; }

private:
  stream_from &m_home;
};
} // namespace pqxx::internal

#include "pqxx/internal/compiler-internal-post.hxx"
#endif
