#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>
#include <iterator>

#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Run a query and try various standard C++
// algorithms on its result.
namespace
{
// Adds first element of each row it receives to a container
template<typename CONTAINER> struct Add
{
  CONTAINER &Container;
  std::string Key;

  Add(std::string K, CONTAINER &C) : Container(C), Key(K) {}

  void operator()(pqxx::row const &T) { Container.push_back(T[Key].c_str()); }
};


template<typename CONTAINER>
Add<CONTAINER> AdderFor(std::string K, CONTAINER &C)
{
  return Add<CONTAINER>(K, C);
}


struct Cmp
{
  using first_argument_type = pqxx::row;
  using second_argument_type = pqxx::row;
  using result_type = bool;

  std::string Key;

  explicit Cmp(std::string K) : Key(K) {}

  bool operator()(pqxx::row const &L, pqxx::row const &R) const
  {
    return std::string{L[Key].c_str()} < std::string{R[Key].c_str()};
  }
};


struct CountGreaterSmaller
{
  std::string Key;
  result const &R;

  CountGreaterSmaller(std::string K, result const &X) : Key(K), R(X) {}

  void operator()(pqxx::row const &T) const
  {
    // Count number of entries with key greater/smaller than first row's key
    // using std::count_if<>()
    using namespace std::placeholders;
    auto const Greater{
      std::count_if(std::begin(R), std::end(R), std::bind(Cmp(Key), _1, T))},
      Smaller{
        std::count_if(std::begin(R), std::end(R), std::bind(Cmp(Key), T, _1))};

    // TODO: Use C++20's ssize().
    PQXX_CHECK(
      Greater + Smaller < ptrdiff_t(std::size(R)),
      "More non-equal rows than rows.");
  }
};


void test_049()
{
  connection conn;
  work tx{conn};
  std::string Table{"pg_tables"}, Key{"tablename"};

  result R(tx.exec("SELECT * FROM " + Table + " ORDER BY " + Key));
  PQXX_CHECK(not std::empty(R), "No rows in " + Table + ".");

  // Verify that for each key in R, the number of greater and smaller keys
  // are sensible; use std::for_each<>() to iterate over rows in R
  std::for_each(std::begin(R), std::end(R), CountGreaterSmaller(Key, R));
}


PQXX_REGISTER_TEST(test_049);
} // namespace
