#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/pipeline>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void TestPipeline(pipeline &P, int numqueries)
{
    const string Q("SELECT count(*) FROM pg_tables");

    for (int i=numqueries; i; --i) P.insert(Q);

    if (numqueries && P.empty())
      throw logic_error("Pipeline is inexplicably empty");

    int res = 0;
    for (int i=numqueries; i; --i)
    {
      if (P.empty())
	throw logic_error("Got " + to_string(numqueries-i) + " "
	    "results from pipeline; expected " + to_string(numqueries));

      pair<pipeline::query_id, result> R = P.retrieve();

      cout << "Query #" << R.first << ": " << R.second.at(0).at(0) << endl;
      if (res && (R.second[0][0].as<int>() != res))
	throw logic_error("Expected " + to_string(res) + " out of pipeline, "
	    "got " + R.second[0][0].c_str());
      res = R.second[0][0].as<int>();
    }

    if (!P.empty()) throw logic_error("Pipeline not empty after retrieval!");
}
} // namespace


// Test program for libpqxx.  Issue a query repeatedly through a pipeline, and
// compare results.
//
// Usage: test069 [connect-string]
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
    work W(C, "test69");
    pipeline P(W);

    if (!P.empty()) throw logic_error("Pipeline not empty initially!");

    for (int i=0; i<5; ++i) TestPipeline(P, i);
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

