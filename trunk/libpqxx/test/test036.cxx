#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/result>
#include <pqxx/transactor>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
//
// Usage: test036 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

class ReadTables : public transactor<nontransaction>
{
  result m_Result;
public:
  ReadTables() : transactor<nontransaction>("ReadTables") {}

  void operator()(argument_type &T)
  {
    m_Result = T.Exec("SELECT * FROM pg_tables");
  }

  void OnCommit()
  {
    for (result::const_iterator c = m_Result.begin(); c != m_Result.end(); ++c)
    {
      string N;
      c[0].to(N);

      cout << '\t' << ToString(c.num()) << '\t' << N << endl;
    }
  }
};


int main(int, char *argv[])
{
  try
  {
    lazyconnection C(argv[1]);

    C.Perform(ReadTables());
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

