#include <iostream>

#include <pqxx/connection>
#include <pqxx/robusttransaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
//
// Usage: test016 [connect-string]
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

    // Begin a "non-transaction" acting on our current connection.  This is
    // really all the transactional integrity we need since we're only 
    // performing one query which does not modify the database.
    robusttransaction<> T(C, "test16");

    result R( T.exec("SELECT * FROM pg_tables") );

    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      string N;
      c[0].to(N);

      cout << '\t' << ToString(c.num()) << '\t' << N << endl;
    }

    // "Commit" the non-transaction.  This doesn't really do anything since
    // NonTransaction doesn't start a backend transaction.
    T.commit();
  }
  catch (const sql_error &e)
  {
    // If we're interested in the text of a failed query, we can write separate
    // exception handling code for this type of exception
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

