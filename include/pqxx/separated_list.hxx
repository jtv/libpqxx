/* Helper similar to Python's `str.join()`.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/separated_list instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_SEPARATED_LIST
#define PQXX_H_SEPARATED_LIST

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <numeric>

#include "pqxx/strconv.hxx"

// TODO: Optimise buffer allocation using random_access_range/iterator.
// C++23: Use std::ranges::views::join_with()?
namespace pqxx
{
/**
 * @defgroup utility Utility functions
 */
//@{

/// Represent sequence of values as a string, joined by a given separator.
/**
 * Use this to turn e.g. the numbers 1, 2, and 3 into a string "1, 2, 3".
 *
 * @param sep separator string (to be placed between items)
 * @param begin beginning of items sequence
 * @param end end of items sequence
 * @param access functor defining how to dereference sequence elements
 */
template<std::forward_iterator ITER, typename ACCESS>
[[nodiscard]] inline std::string separated_list(
  std::string_view sep, ITER begin, ITER end, ACCESS access, ctx c = {})
{
  if (end == begin)
    return {};
  auto next{begin};
  ++next;
  if (next == end)
    return to_string(access(begin), c);

  // From here on, we've got at least 2 elements -- meaning that we need sep.

  std::size_t budget{0};
  for (ITER cnt{begin}; cnt != end; ++cnt)
    budget += pqxx::size_buffer(access(cnt));
  budget +=
    static_cast<std::size_t>(std::distance(begin, end)) * std::size(sep);

  std::string result;
  result.resize(budget);

  char *const data{result.data()};
  char *stop{data + budget};
  std::size_t here{pqxx::into_buf({data, stop}, access(begin))};
  for (++begin; begin != end; ++begin)
  {
    here = pqxx::internal::copy_chars<false>(sep, result, here, sl::current());
    here += pqxx::into_buf({data + here, stop}, access(begin));
  }
  result.resize(here);
  return result;
}


/// Render sequence as a string, using given separator between items.
template<std::forward_iterator ITER>
[[nodiscard]] inline std::string
separated_list(std::string_view sep, ITER begin, ITER end, ctx c = {})
{
  return separated_list(sep, begin, end, [](ITER i) { return *i; }, c);
}


/// Render items in a container as a string, using given separator.
template<std::ranges::range CONTAINER>
[[nodiscard]] inline std::string
separated_list(std::string_view sep, CONTAINER &&con, ctx c = {})
{
  return separated_list(sep, std::begin(con), std::end(con), c);
}


/// Render items in a tuple as a string, using given separator.
template<typename TUPLE, std::size_t INDEX = 0, typename ACCESS>
[[nodiscard]] inline std::string separated_list(
  std::string_view sep, TUPLE const &t, ACCESS const &access, ctx c = {})
{
  std::string out{to_string(access(&std::get<INDEX>(t)), c)};
  if constexpr (INDEX < std::tuple_size<TUPLE>::value - 1)
  {
    out.append(sep);
    out.append(separated_list<TUPLE, INDEX + 1>(sep, t, access, c));
  }
  return out;
}

template<typename TUPLE, std::size_t INDEX = 0>
[[nodiscard]] inline std::string
separated_list(std::string_view sep, TUPLE const &t, ctx c = {})
{
  return separated_list(sep, t, [](TUPLE const &tup) { return *tup; }, c);
}
//@}
} // namespace pqxx
#endif
