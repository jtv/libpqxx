#include <iostream>
#include <sstream>

#include <pqxx/all.h>

using namespace PGSTD;
using namespace pqxx;


// Streams test program for libpqxx.  Insert a result field into various
// types of streams.
//
// Usage: test046 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    Transaction T(C, "test46");
    result R( T.Exec("SELECT count(*) FROM pg_tables") );

    cout << "Count was " << R.at(0).at(0) << endl;

    // Read the value into a stringstream
    stringstream I;
    I << R[0][0];

    // Now convert the stringstream into a numeric type
    long L;
    I >> L;
    cout << "As a long, it's " << L << endl;

    long L2;
    R[0][0].to(L2);
    if (L != L2)
      throw logic_error("Different conversion methods gave different results!");
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
    return 1;
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }

  return 0;
}


