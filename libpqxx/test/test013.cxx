#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/transactor>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of transactor.
//
// Usage: test013 [connect-string] [table]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
//
// Note for the superstitious: the numbering for this test program is pure
// coincidence.


namespace
{

// Let's take a boring year that is not going to be in the "pqxxevents" table
const int BoringYear = 1977;


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
class CountEvents : public transactor<>
{
  string m_Table;
  pair<int, int> &m_Results;
public:
  CountEvents(string Table, pair<int,int> &Results) : 
    transactor<>("CountEvents"), m_Table(Table), m_Results(Results) {}

  void operator()(argument_type &T)
  {
    const string CountQuery = "SELECT count(*) FROM " + m_Table;
    result R;

    R = T.exec(CountQuery);
    R.at(0).at(0).to(m_Results.first);

    R = T.exec(CountQuery + " WHERE year=" + ToString(BoringYear));
    R.at(0).at(0).to(m_Results.second);
  }
};



class FailedInsert : public transactor<>
{
  string m_Table;
public:
  FailedInsert(string Table) : 
    transactor<>("FailedInsert"),
    m_Table(Table)
  {
  }

  void operator()(argument_type &T)
  {
    result R( T.exec("INSERT INTO " + m_Table + " VALUES (" +
	             ToString(BoringYear) + ", "
	             "'yawn')") );

    assert(R.AffectedRows() == 1);
    cout << "Inserted row with oid " << R.InsertedOid() << endl;

    throw runtime_error("Transaction deliberately aborted");
  }

  void OnAbort(const char Reason[]) throw ()
  {
    cerr << "(Expected) Transactor " << Name() << " failed: " << Reason << endl;
  }

  void OnCommit()
  {
    cerr << "Transactor " << Name() << " succeeded." << endl;
  }

  void OnDoubt() throw ()
  {
    cerr << "Transactor " << Name() << " in indeterminate state!" << endl;
  }
};


} // namespace


int main(int argc, char *argv[])
{
  try
  {
    connection C(argv[1]);

    const string Table = ((argc > 2) ? argv[2] : "pqxxevents");

    pair<int,int> Before;
    C.perform(CountEvents(Table, Before));
    if (Before.second) 
      throw runtime_error("Table already has an event for " + 
		          ToString(BoringYear) + ", "
			  "cannot run.");

    const FailedInsert DoomedTransaction(Table);

    try
    {
      C.perform(DoomedTransaction);
    }
    catch (const exception &e)
    {
      cerr << "(Expected) Doomed transaction failed: " << e.what() << endl;
    }

    pair<int,int> After;
    C.perform(CountEvents(Table, After));

    if (After != Before)
      throw logic_error("Event counts changed from "
			"{" + ToString(Before.first) + "," + 
			ToString(Before.second) + "} "
			"to "
			"{" + ToString(After.first) + "," +
			ToString(After.second) + "} "
		        "despite abort.  This could be a bug in libpqxx, "
			"or something else modified the table.");
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

