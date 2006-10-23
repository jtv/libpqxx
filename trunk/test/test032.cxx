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
// Usage: test032 [connect-string] [table]
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


// Function to print database's warnings  to cerr
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

    R = T.exec(CountQuery + " WHERE year=" + to_string(BoringYear));
    R.at(0).at(0).to(m_Results.second);
  }
};



class FailedInsert : public transactor<>
{
  string m_Table;
  static string LastReason;
public:
  FailedInsert(string Table) :
    transactor<>("FailedInsert"),
    m_Table(Table)
  {
  }

  void operator()(argument_type &T)
  {
    T.exec("INSERT INTO " + m_Table + " VALUES (" +
	    to_string(BoringYear) + ", "
	    "'yawn')");

    throw runtime_error("Transaction deliberately aborted");
  }

  void on_abort(const char Reason[]) throw ()
  {
    if (Reason != LastReason)
    {
      cout << "(Expected) Transactor " << Name() << " failed: "
	    << Reason << endl;
      LastReason = Reason;
    }
  }

  void on_commit()
  {
    cerr << "Transactor " << Name() << " succeeded." << endl;
  }

  void on_doubt() throw ()
  {
    cerr << "Transactor " << Name() << " in indeterminate state!" << endl;
  }
};

string FailedInsert::LastReason;

} // namespace


int main(int argc, char *argv[])
{
  try
  {
    lazyconnection C(argv[1]);

    const string Table = ((argc > 2) ? argv[2] : "pqxxevents");

    pair<int,int> Before;
    C.perform(CountEvents(Table, Before));
    if (Before.second)
      throw runtime_error("Table already has an event for " +
		          to_string(BoringYear) + ", "
			  "cannot run.");

    const FailedInsert DoomedTransaction(Table);

    try
    {
      disable_noticer d(C);
      C.perform(DoomedTransaction);
    }
    catch (const exception &e)
    {
      cout << "(Expected) Doomed transaction failed: " << e.what() << endl;
    }

    pair<int,int> After;
    C.perform(CountEvents(Table, After));

    if (After != Before)
      throw logic_error("Event counts changed from "
			"{" + to_string(Before.first) + "," +
			to_string(Before.second) + "} "
			"to "
			"{" + to_string(After.first) + "," +
			to_string(After.second) + "} "
		        "despite abort.  This could be a bug in libpqxx, "
			"or something else modified the table.");
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

