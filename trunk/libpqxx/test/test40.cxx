#include <cassert>
#include <iostream>

#include <pqxx/cachedresult.h>
#include <pqxx/connection.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Compare behaviour of a CachedResult to a regular
// Result.
//
// Usage: test40 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    // Set up a connection to the backend
    Connection C(argv[1] ? argv[1] : "");

    // Begin a transaction acting on our current connection
    Transaction T(C, "test40");

    const char Query[] = "SELECT * FROM events";

    // Perform a query on the database, storing result tuples in R
    Result R( T.Exec(Query) );

    for (int BlockSize = 2; BlockSize <= R.size()+1; ++BlockSize)
    {
      CachedResult CR(T, Query, "cachedresult", BlockSize);
      const CachedResult::size_type CRS = CR.size();
      if (CRS != R.size())
	throw logic_error("BlockSize " + ToString(BlockSize) + ": "
	                  "Expected " + ToString(R.size()) + " rows, "
			  "got " + ToString(CRS));
      if (CR.size() != CRS)
	throw logic_error("BlockSize " + ToString(BlockSize) + ": "
	                  "Inconsistent size (" + ToString(CRS) + " vs. " +
			  ToString(CR.size()) + ")");

      // Compare contents for CR with R
      for (Result::size_type i = 0; i < R.size(); ++i)
      {
	string A, B;
	R.at(i).at(0).to(A);
	CR.at(i).at(0).to(B);
	if (A != B)
	  throw logic_error("BlockSize " + ToString(BlockSize) + ", "
	                    "row " + ToString(i) + ": "
			    "Expected '" + A + "', "
			    "got '" + B + "'");
      }

      // TODO: Wilder navigation (reverse?), random access
    }
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


