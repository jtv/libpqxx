#include <cassert>
#include <iostream>

#include <pqxx/connection.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open a lazy connection to database, start
// a transaction, and perform a query inside it.
//
// Usage: test21 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    const string ConnectString = (argv[1] ? argv[1] : "");
    // Request a connection to the backend, but defer actual creation
    LazyConnection C(ConnectString);

    C.ProcessNotice("Printing details on deferred connection\n");
    const string HostName = (C.HostName() ? C.HostName() : "<local>");
    C.ProcessNotice(string() +
		    "database=" + C.DbName() + ", "
		    "username=" + C.UserName() + ", "
		    "hostname=" + HostName + ", "
		    "port=" + ToString(C.Port()) + ", "
		    "options='" + C.Options() + "', "
		    "backendpid=" + ToString(C.BackendPID()) + "\n");

    Transaction T(C, "test21");

    // By now our connection should really have been created
    C.ProcessNotice("Printing details on actual connection\n");
    C.ProcessNotice(string() +
		    "database=" + C.DbName() + ", "
		    "username=" + C.UserName() + ", "
		    "hostname=" + HostName + ", "
		    "port=" + ToString(C.Port()) + ", "
		    "options='" + C.Options() + "', "
		    "backendpid=" + ToString(C.BackendPID()) + "\n");


    Result R( T.Exec("SELECT * FROM pg_tables") );

    T.ProcessNotice(ToString(R.size()) + " "
		    "result tuples in transaction " +
		    T.Name() +
		    "\n");

    // Process each successive result tuple
    for (Result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      string N;
      c[0].to(N);

      cout << '\t' << ToString(c.num()) << '\t' << N << endl;
    }

    T.Commit();
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
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}

