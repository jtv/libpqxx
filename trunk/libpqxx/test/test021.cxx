#include <cassert>
#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open a lazy connection to database, start
// a transaction, and perform a query inside it.
//
// Usage: test021 [connect-string]
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
    lazyconnection C(ConnectString);

    C.process_notice("Printing details on deferred connection\n");
    const string HostName = (C.hostname() ? C.hostname() : "<local>");
    C.process_notice(string() +
		     "database=" + C.dbname() + ", "
		     "username=" + C.username() + ", "
		     "hostname=" + HostName + ", "
		     "port=" + to_string(C.port()) + ", "
		     "options='" + C.options() + "', "
		     "backendpid=" + to_string(C.backendpid()) + "\n");

    work T(C, "test21");

    // By now our connection should really have been created
    C.process_notice("Printing details on actual connection\n");
    C.process_notice(string() +
		     "database=" + C.dbname() + ", "
		     "username=" + C.username() + ", "
		     "hostname=" + HostName + ", "
		     "port=" + to_string(C.port()) + ", "
		     "options='" + C.options() + "', "
		     "backendpid=" + to_string(C.backendpid()) + "\n");


    result R( T.exec("SELECT * FROM pg_tables") );

    T.process_notice(to_string(R.size()) + " "
		     "result tuples in transaction " +
		     T.name() +
		     "\n");

    // Process each successive result tuple
    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      string N;
      c[0].to(N);

      cout << '\t' << to_string(c.num()) << '\t' << N << endl;
    }

    T.commit();
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

