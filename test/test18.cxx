#include <functional>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of RobustTransaction.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
namespace
{

// Let's take a boring year that is not going to be in the "pqxxevents" table
const long BoringYear = 1977;


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
pair<int, int> count_events(connection_base &C, string table)
{
  nontransaction tx{C};
  const string CountQuery = "SELECT count(*) FROM " + table;
  int all_years, boring_year;
  row R;

  R = tx.exec1(CountQuery);
  R.front().to(all_years);

  R = tx.exec1(CountQuery + " WHERE year=" + to_string(BoringYear));
  R.front().to(boring_year);
  return make_pair(all_years, boring_year);
};


struct deliberate_error : exception
{
};


void test_018(transaction_base &T)
{
  connection_base &C(T.conn());
  T.abort();

  {
    work T2(C);
    test::create_pqxxevents(T2);
    T2.commit();
  }

  const string Table = "pqxxevents";

  const pair<int,int> Before = perform(bind(count_events, ref(C), Table));
  PQXX_CHECK_EQUAL(
	Before.second,
	0,
	"Already have event for " + to_string(BoringYear) + ", cannot run.");

  {
    quiet_errorhandler d(C);
    PQXX_CHECK_THROWS(
	perform(
          [&C, Table]()
          {
            robusttransaction<serializable> tx{C};
            tx.exec0(
		"INSERT INTO " + Table + " VALUES (" +
		to_string(BoringYear) + ", '" + tx.esc("yawn") + "')");

            throw deliberate_error();
          }),
	deliberate_error,
	"Not getting expected exception from failing transactor.");
  }

  const pair<int,int> After = perform(bind(count_events, ref(C), Table));

  PQXX_CHECK_EQUAL(After.first, Before.first, "Event count changed.");
  PQXX_CHECK_EQUAL(
	After.second,
	Before.second,
	"Event count for " + to_string(BoringYear) + " changed.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_018, nontransaction)
