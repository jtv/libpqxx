#include <algorithm>
#include <cstdio>
#include <functional>
#include <iostream>
#include <iterator>

#include <pqxx/pqxx>

// For count_if workaround
#include <pqxx/config-internal-compiler.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

#ifndef PQXX_HAVE_COUNT_IF
template<typename IN, typename P> inline long count_if(IN first, IN last, P p)
{
  long cnt;
  for (cnt=0; first!=last; ++first) cnt += p(*first);
  return cnt;
}
#endif


// Adds first element of each tuple it receives to a container
template<typename CONTAINER> struct Add
{
  CONTAINER &Container;
  string Key;

  Add(string K, CONTAINER &C) : Container(C), Key(K) {}

  void operator()(const result::tuple &T)
  {
    Container.push_back(T[Key].c_str());
  }
};


template<typename CONTAINER>
Add<CONTAINER> AdderFor(string K, CONTAINER &C)
{
  return Add<CONTAINER>(K, C);
}


struct Cmp : binary_function<result::tuple, result::tuple, bool>
{
  string Key;

  explicit Cmp(string K) : Key(K) {}

  bool operator()(const result::tuple &L, const result::tuple &R) const
  {
    return string(L[Key].c_str()) < string(R[Key].c_str());
  }
};

struct CountGreaterSmaller : unary_function<result::tuple, void>
{
  string Key;
  const result &R;

  CountGreaterSmaller(string K, const result &X) : Key(K), R(X) {}

  void operator()(const result::tuple &T) const
  {
    // Count number of entries with key greater/smaller than first row's key
    // using std::count_if<>()
    const result::size_type
      Greater = count_if(R.begin(), R.end(), bind2nd(Cmp(Key),T)),
      Smaller = count_if(R.begin(), R.end(), bind1st(Cmp(Key),T));

    cout << "'" << T[Key] << "': "
         << Greater << " greater, "
         << Smaller << " smaller "
	 << "(" << (Greater + Smaller) << " total)"
	 << endl;

    if (Greater + Smaller >= R.size())
      throw logic_error("Of " + to_string(R.size()) + " keys, " +
	                to_string(Greater) + " were greater than '" +
			string(T[Key].c_str()) + "' and " +
			to_string(Smaller) + " were smaller--that's too many!");
  }
};

} // namespace


// Test program for libpqxx.  Run a query and try various standard C++
// algorithms on it.
//
// Usage: test049 [connect-string] [tablename key]
//
// The connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The tablename / key combination defines which table to query, and a
// field (which must be a text field in this case) to sort by.

int main(int argc, char *argv[])
{
  try
  {
    string Table="pg_tables", Key="tablename";

    switch (argc)
    {
    case 0: throw logic_error("argc is zero!");
    case 1: break; // OK, no arguments
    case 2: break; // OK, just a connection string

    case 4:
      Table = argv[2];
      Key = argv[3];
      break;

    case 3:
    default:
      throw invalid_argument("Usage: test049 [connectstring] [tablename key]");
    }

    connection C(argv[1]);
    work T(C, "test49");

    result R( T.exec("SELECT * FROM " + Table + " ORDER BY " + Key) );
    cout << "Read " << R.size() << " tuples." << endl;
    if (R.empty()) throw runtime_error("No entries in table '" + Table + "'!");

    // Verify that for each key in R, the number of greater and smaller keys
    // are sensible; use std::for_each<>() to iterate over rows in R
    for_each(R.begin(), R.end(), CountGreaterSmaller(Key, R));
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
    return 1;
  }
  catch (const exception &e)
  {
    // All exceptions thrown by libpqxx are derived from std::exception
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    // This is really unexpected (see above)
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}

