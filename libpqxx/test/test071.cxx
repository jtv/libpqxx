#include <iostream>
#include <map>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/pipeline>

using namespace PGSTD;
using namespace pqxx;

typedef map<pipeline::query_id, int> Exp;

template<typename MAPIT>
void checkresult(pipeline &P, MAPIT c)
{
  const result r = P.retrieve(c->first);
  const int val = r.at(0).at(0).as(int(0));
  if (val != c->second)
    throw logic_error("Query #" + ToString(c->first) + ": "
	"expected result " + ToString(c->second) + ", "
	"got " + r[0][0].c_str());
}

// Test program for libpqxx.  Issue queries through a pipeline, and retrieve
// results both in-order and out-of-order.
//
// Usage: test071 [connect-string]
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
    work W(C, "test71");
    pipeline P(W);

    // Keep expected result for every query we issue
    Exp values;

    // Insert queries returning various numbers
    for (int i=1; i<10; ++i) values[P.insert("SELECT " + ToString(i))] = i;

    // Retrieve results in query_id order, and compare to expected values
    for (Exp::const_iterator c=values.begin(); c!=values.end(); ++c)
      checkresult(P, c);

    if (!P.empty())
      throw logic_error("Pipeline not empty after all values retrieved");

    values.clear();

    // Insert more queries returning various numbers
    P.retain();
    for (int i=100; i>90; --i) values[P.insert("SELECT " + ToString(i))] = i;

    // See that all queries are issued on resume()
    P.resume();
    for (Exp::const_iterator c=values.begin(); c!=values.end(); ++c)
      if (!P.is_running(c->first))
	throw logic_error("Query #" + ToString(c->first) + " "
	    "not running after resume()");

    // Retrieve results in reverse order
    for (Exp::reverse_iterator c=values.rbegin(); c!=values.rend(); ++c)
      checkresult(P, c);

    values.clear();
    P.retain();
    for (int i=1010; i>1000; --i) values[P.insert("SELECT " + ToString(i))] = i;
    for (Exp::const_iterator c=values.begin(); c!=values.end(); ++c)
    {
      if (P.is_running(c->first))
	cout << "Query #" << c->first << " issued despite retain()" << endl;
      if (P.is_finished(c->first))
	cout << "Query #" << c->first << " completed despite retain()" << endl;
    }

    // See that all results are retrieved by complete()
    P.complete();
    for (Exp::const_iterator c=values.begin(); c!=values.end(); ++c)
      if (!P.is_running(c->first))
	throw logic_error("Query #" + ToString(c->first) + " "
	    "not finished after complete()");
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

