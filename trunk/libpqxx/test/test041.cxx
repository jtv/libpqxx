#include <cassert>
#include <iostream>

#include <pqxx/cachedresult.h>
#include <pqxx/connection.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

// Verify that CachedResult::at() catches an index overrun
void CheckOverrun(const CachedResult &CR, 
                  CachedResult::size_type Overrun,
		  string &LastReason)
{
  const CachedResult::size_type Base = ((Overrun >= 0) ? CR.size() : 0);

  bool OK = false;
  string Entry;
  try
  {
    CR.at(Base + Overrun).at(0).to(Entry);
  }
  catch (const exception &e)
  {
    // OK, this is what we expected to happen
    OK = true;
    if (LastReason != e.what())
    {
      cerr << "(Expected) " << e.what() << endl;
      LastReason = e.what();

    }
  }

  if (!OK) 
    throw logic_error("Failed to detect overrun "
	              "(row " + ToString(Base + Overrun) + "); "
		      "found '" + Entry + "'");
}

}


// Test program for libpqxx.  Compare behaviour of a CachedResult to a regular
// Result.
//
// Usage: test41 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    Connection C(argv[1]);
    Transaction T(C, "test41");

    const char Query[] = "SELECT * FROM events ORDER BY year";

    Result R( T.Exec(Query) );
    string Msg;

    for (int BlockSize = 2; BlockSize <= R.size()+1; ++BlockSize)
    {
      CachedResult CR(T, Query, "cachedresult", BlockSize);
 
      // Verify that we get an exception if we exceed CR's range, and are able
      // to recover afterwards
      for (CachedResult::size_type n = -2; n < 2; ++n) CheckOverrun(CR, n, Msg);

      // Compare contents for CR with R
      for (Result::size_type i = R.size() - 1; i >= 0 ; --i)
      {
	string A, B;
	R.at(i).at(0).to(A);
	CR.at(i).at(0).to(B);

	if (A != B)
	  throw logic_error("BlockSize " + ToString(BlockSize) + ", "
	                    "row " + ToString(i) + ": "
			    "Expected '" + A + "', "
			    "got '" + B + "'");

	CR[i][0].to(B);
	if (A != B)
	  throw logic_error("BlockSize " + ToString(BlockSize) + ", "
	                    "row " + ToString(i) + ": "
			    "at() gives '" + A + "', "
			    "[] gives '" + B + "'");
      }
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


