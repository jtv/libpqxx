#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection.h>
#include <pqxx/nontransaction.h>
#include <pqxx/result.h>
#include <pqxx/robusttransaction.h>
#include <pqxx/transaction.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of RobustTransaction with
// a lazy connection.
//
// Usage: test37 [connect-string] [table]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The program will attempt to add an entry to a table called "events",
// with a key column called "year"--and then abort the change.


namespace
{
extern "C"
{
void ReportWarning(void *, const char msg[])
{
  cerr << msg;
}
}


// Let's take a boring year that is not going to be in the "events" table
const int BoringYear = 1977;


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
class CountEvents : public Transactor
{
  string m_Table;
  pair<int, int> &m_Results;
public:
  typedef NonTransaction argument_type;

  CountEvents(string Table, pair<int,int> &Results) : 
    Transactor("CountEvents"), m_Table(Table), m_Results(Results) {}

  void operator()(argument_type &T)
  {
    const string CountQuery = "SELECT count(*) FROM " + m_Table;
    Result R;

    R = T.Exec(CountQuery.c_str());
    R.at(0).at(0).to(m_Results.first);

    R = T.Exec((CountQuery + " WHERE year=" + ToString(BoringYear)).c_str());
    R.at(0).at(0).to(m_Results.second);
  }
};



class FailedInsert : public Transactor
{
  string m_Table;
public:
  typedef RobustTransaction argument_type;

  FailedInsert(string Table) : 
    Transactor("FailedInsert"), 
    m_Table(Table)
  {
  }

  void operator()(argument_type &T)
  {
    T.Exec(("INSERT INTO " + m_Table + " VALUES (" +
	    ToString(BoringYear) + ", "
	    "'yawn')").c_str());

    throw runtime_error("Transaction deliberately aborted");
  }

  void OnAbort(const char Reason[]) throw ()
  {
    cerr << "Transactor " << Name() << " failed: " << Reason << endl;
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
    Connection C(argv[1] ? argv[1] : "", false);
    C.SetNoticeProcessor(ReportWarning, 0);

    const string Table = ((argc > 2) ? argv[2] : "events");

    pair<int,int> Before;
    C.Perform(CountEvents(Table, Before));
    if (Before.second) 
      throw runtime_error("Table already has an event for " + 
		          ToString(BoringYear) + ", "
			  "cannot run.");

    const FailedInsert DoomedTransaction(Table);

    try
    {
      C.Perform(DoomedTransaction);
    }
    catch (const exception &e)
    {
      cerr << "Doomed transaction failed: " << e.what() << endl;
    }

    pair<int,int> After;
    C.Perform(CountEvents(Table, After));

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

