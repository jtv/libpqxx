#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/pipeline>

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test error handling for pipeline.
//
// Usage: test072 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend
// running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    asyncconnection C(argv[1]);
    nontransaction W(C, "test72");
    pipeline P(W);

    // Ensure all queries are issued at once to make the test more interesting
    P.retain();

    // The middle query should fail; the surrounding two should succeed
    const pipeline::query_id id_1 = P.insert("SELECT 1");
    const pipeline::query_id id_f = P.insert("SELECT * FROM pg_nonexist");
    const pipeline::query_id id_2 = P.insert("SELECT 2");

    // See that we can process the queries without stumbling over the error
    P.complete();

    // We should be able to get the first result, which preceeds the error
    cout << "Retrieving initial result..." << endl;
    const int res_1 = P.retrieve(id_1).at(0).at(0).as<int>();
    cout << " - result was " << res_1 << endl;
    if (res_1 != 1) throw logic_error("Expected 1, got " + to_string(res_1));

    // We should *not* get a result for the query behind the error
    cout << "Restrieving post-error result..." << endl;
    bool failed = true;
    try
    {
      disable_noticer d(C);
      P.retrieve(id_2).at(0).at(0).as<int>();
      failed = false;
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    if (!failed) throw logic_error("Pipeline wrongly resumed after SQL error");

    // Now see that we get an exception when we touch the failed result
    cout << "Retrieving result for failed query..." << endl;
    try
    {
      P.retrieve(id_f);
      failed = false;
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
      failed = true;
    }
    if (!failed) throw logic_error("Pipeline failed to register SQL error");
  }
  catch (const sql_error &e)
  {
    cerr << "Database error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 2;
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

