#include <functional>

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

pair<int, int> count_events(connection_base &conn, string table)
{
  const string CountQuery = "SELECT count(*) FROM " + table;
  row R;
  int all_years, boring_year;

  work tx{conn};
  R = tx.exec1(CountQuery);
  R.front().to(all_years);

  R = tx.exec1(CountQuery + " WHERE year=" + to_string(BoringYear));
  R.front().to(boring_year);

  return make_pair(all_years, boring_year);
}


struct deliberate_error : exception
{
};


void test_032()
{
  lazyconnection conn;
  {
    nontransaction tx{conn};
    test::create_pqxxevents(tx);
  }

  const string Table = "pqxxevents";

  const pair<int,int> Before = perform(bind(count_events, ref(conn), Table));
  PQXX_CHECK_EQUAL(
	Before.second,
	0,
	"Already have event for " + to_string(BoringYear) + ", cannot test.");

  {
    quiet_errorhandler d(conn);
    PQXX_CHECK_THROWS(
	perform(
          [&conn, &Table]()
          {
            work{conn}.exec0(
		"INSERT INTO " + Table + " VALUES (" +
		to_string(BoringYear) + ", "
		"'yawn')");
            throw deliberate_error();
          }),
	deliberate_error,
	"Did not get expected exception from failing transactor.");
  }

  const pair<int,int> After = perform(bind(count_events, ref(conn), Table));

  PQXX_CHECK_EQUAL(After.first, Before.first, "Event count changed.");
  PQXX_CHECK_EQUAL(
	After.second,
	 Before.second,
	 "Event count for " + to_string(BoringYear) + " changed.");
}


PQXX_REGISTER_TEST(test_032);
} // namespace
