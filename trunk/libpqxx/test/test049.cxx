#include <algorithm>
#include <cstdio>
#include <iostream>
#include <vector>

#include <pqxx/all.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

// Adds first element of each Tuple it receives to a container
template<typename CONTAINER> struct Add
{
  CONTAINER &Container;
  explicit Add(CONTAINER &C) : Container(C) {}
  void operator()(const Result::Tuple &T) 
  { 
    Container.push_back(T[0].c_str()); 
  }
};


template<typename CONTAINER> Add<CONTAINER> AdderFor(CONTAINER &C)
{
  return Add<CONTAINER>(C);
}

} // namespace


// Test program for libpqxx.  Run a query and try various standard C++
// algorithms on it.
//
// Usage: test049 [connect-string] [table]
//
// Where table is the table to be queried; if none is given, pg_tables is
// queried by default.
//
// The connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int argc, char *argv[])
{
  try
  {
    const string Table = ((argc >= 3) ? argv[2] : "pg_tables");

    Connection C(argv[1]);
    Transaction T(C, "test49");

    Result R( T.Exec("SELECT * FROM " + Table) );

    vector<string> V;
    for_each(R.begin(), R.end(), AdderFor(V));
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

