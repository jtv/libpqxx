#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of transactor.
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

    R = T.exec(CountQuery + " WHERE year=" + to_string(BoringYear));
    R.at(0).at(0).to(m_Results.second);
  }
};


struct deliberate_error : exception
{
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

    throw deliberate_error();
  }

  void on_abort(const char Reason[]) PQXX_NOEXCEPT
  {
    if (Reason != LastReason)
    {
      pqxx::test::expected_exception(
	"Transactor " + Name() + " failed: " + Reason);
      LastReason = Reason;
    }
  }
};


string FailedInsert::LastReason;


void test_032(transaction_base &)
{
  lazyconnection C;
  {
    nontransaction W(C);
    test::create_pqxxevents(W);
  }

  const string Table = "pqxxevents";

  pair<int,int> Before;
  C.perform(CountEvents(Table, Before));
  PQXX_CHECK_EQUAL(
	Before.second,
	0,
	"Already have event for " + to_string(BoringYear) + ", cannot test.");

  const FailedInsert DoomedTransaction(Table);

  {
    quiet_errorhandler d(C);
    PQXX_CHECK_THROWS(
	C.perform(DoomedTransaction),
	deliberate_error, 
	"Did not get expected exception from failing transactor.");
  }

  pair<int,int> After;
  C.perform(CountEvents(Table, After));

  PQXX_CHECK_EQUAL(After.first, Before.first, "Event count changed.");
  PQXX_CHECK_EQUAL(
	After.second,
	 Before.second,
	 "Event count for " + to_string(BoringYear) + " changed.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_032)
