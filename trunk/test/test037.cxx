#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/result>
#include <pqxx/robusttransaction>
#include <pqxx/transactor>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of RobustTransaction with
// a lazy connection.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
const int BoringYear = 1977;


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
class CountEvents : public transactor<nontransaction>
{
  string m_Table;
  pair<int, int> &m_Results;
public:
  CountEvents(string Table, pair<int,int> &Results) :
    transactor<nontransaction>("CountEvents"),
    m_Table(Table),
    m_Results(Results)
  {
  }

  void operator()(argument_type &T)
  {
    const string CountQuery = "SELECT count(*) FROM " + m_Table;
    result R;

    R = T.exec(CountQuery.c_str());
    R.at(0).at(0).to(m_Results.first);

    R = T.exec(CountQuery + " WHERE year=" + to_string(BoringYear));
    R.at(0).at(0).to(m_Results.second);
  }
};


class FailedInsert : public transactor<robusttransaction<> >
{
  string m_Table;
  static string LastReason;
public:
  FailedInsert(string Table) :
    transactor<argument_type>("FailedInsert"),
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


void test_037(connection_base &, transaction_base &)
{
    lazyconnection C;

    const string Table = "pqxxevents";

    pair<int,int> Before;
    C.perform(CountEvents(Table, Before));
    PQXX_CHECK_EQUAL(
	Before.second,
	0,
	"Already have event for " + to_string(BoringYear) + ", cannot test.");

    const FailedInsert DoomedTransaction(Table);

    {
      disable_noticer d(C);
      PQXX_CHECK_THROWS(
	C.perform(DoomedTransaction),
	runtime_error,
	"Did not get expected exception from failing transactor.");
    }

    pair<int,int> After;
    C.perform(CountEvents(Table, After));

    PQXX_CHECK_EQUAL(After.first, Before.first, "Number of events changed.");
    PQXX_CHECK_EQUAL(
	After.second,
	Before.second,
	"Number of events for " + to_string(BoringYear) + " changed.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_037)
