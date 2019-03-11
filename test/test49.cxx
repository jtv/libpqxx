#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>
#include <iterator>

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

  void operator()(const pqxx::row &T)
  {
    Container.push_back(T[Key].c_str());
  }
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

  bool operator()(const pqxx::row &L, const pqxx::row &R) const
  {
    return std::string{L[Key].c_str()} < std::string{R[Key].c_str()};
  }
};


struct CountGreaterSmaller
{
  std::string Key;
  const result &R;

  CountGreaterSmaller(std::string K, const result &X) : Key(K), R(X) {}

  void operator()(const pqxx::row &T) const
  {
    // Count number of entries with key greater/smaller than first row's key
    // using std::count_if<>()
    using namespace std::placeholders;
    const auto
      Greater = std::count_if(R.begin(), R.end(), std::bind(Cmp(Key), _1, T)),
      Smaller = std::count_if(R.begin(), R.end(), std::bind(Cmp(Key), T, _1));

    std::cout
	<< "'" << T[Key] << "': "
	<< Greater << " greater, "
	<< Smaller << " smaller "
	<< "(" << (Greater + Smaller) << " total)"
	<< std::endl;

    PQXX_CHECK(
	Greater + Smaller < ptrdiff_t(R.size()),
	"More non-equal rows than rows.");
  }
};


void test_049()
{
  connection conn;
  work tx{conn};
  std::string Table="pg_tables", Key="tablename";

  result R( tx.exec("SELECT * FROM " + Table + " ORDER BY " + Key) );
  std::cout << "Read " << R.size() << " rows." << std::endl;
  PQXX_CHECK(not R.empty(), "No rows in " + Table + ".");

  // Verify that for each key in R, the number of greater and smaller keys
  // are sensible; use std::for_each<>() to iterate over rows in R
  std::for_each(R.begin(), R.end(), CountGreaterSmaller(Key, R));
}


PQXX_REGISTER_TEST(test_049);
} // namespace
