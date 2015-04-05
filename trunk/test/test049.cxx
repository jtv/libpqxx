#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>
#include <iterator>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Run a query and try various standard C++
// algorithms on it.
namespace
{

// Adds first element of each row it receives to a container
template<typename CONTAINER> struct Add
{
  CONTAINER &Container;
  string Key;

  Add(string K, CONTAINER &C) : Container(C), Key(K) {}

  void operator()(const pqxx::row &T)
  {
    Container.push_back(T[Key].c_str());
  }
};


template<typename CONTAINER>
Add<CONTAINER> AdderFor(string K, CONTAINER &C)
{
  return Add<CONTAINER>(K, C);
}


struct Cmp : binary_function<pqxx::row, pqxx::row, bool>
{
  string Key;

  explicit Cmp(string K) : Key(K) {}

  bool operator()(const pqxx::row &L, const pqxx::row &R) const
  {
    return string(L[Key].c_str()) < string(R[Key].c_str());
  }
};


struct CountGreaterSmaller : unary_function<pqxx::row, void>
{
  string Key;
  const result &R;

  CountGreaterSmaller(string K, const result &X) : Key(K), R(X) {}

  void operator()(const pqxx::row &T) const
  {
    // Count number of entries with key greater/smaller than first row's key
    // using std::count_if<>()
    const ptrdiff_t
      Greater = count_if(R.begin(), R.end(), bind2nd(Cmp(Key),T)),
      Smaller = count_if(R.begin(), R.end(), bind1st(Cmp(Key),T));

    cout << "'" << T[Key] << "': "
         << Greater << " greater, "
         << Smaller << " smaller "
	 << "(" << (Greater + Smaller) << " total)"
	 << endl;

    PQXX_CHECK(
	Greater + Smaller < ptrdiff_t(R.size()),
	"More non-equal rows than rows.");
  }
};


void test_049(transaction_base &T)
{
  string Table="pg_tables", Key="tablename";

  result R( T.exec("SELECT * FROM " + Table + " ORDER BY " + Key) );
  cout << "Read " << R.size() << " rows." << endl;
  PQXX_CHECK(!R.empty(), "No rows in " + Table + ".");

  // Verify that for each key in R, the number of greater and smaller keys
  // are sensible; use std::for_each<>() to iterate over rows in R
  for_each(R.begin(), R.end(), CountGreaterSmaller(Key, R));
}
} // namespace

PQXX_REGISTER_TEST(test_049)
