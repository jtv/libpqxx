#include <cstdio>
#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Open connection to database, start a transaction,
// abort it, and verify that it "never happened."

namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
const int BoringYear = 1977;

const string Table("pqxxevents");


// Count events, and boring events, in table
pair<int,int> CountEvents(transaction_base &T)
{
  const string EventsQuery = "SELECT count(*) FROM " + Table;
  const string BoringQuery = EventsQuery +
		             " WHERE year=" +
			     to_string(BoringYear);
  int EventsCount = 0,
      BoringCount = 0;

  result R( T.exec(EventsQuery) );
  R.at(0).at(0).to(EventsCount);

  R = T.exec(BoringQuery);
  R.at(0).at(0).to(BoringCount);

  return make_pair(EventsCount, BoringCount);
}



// Try adding a record, then aborting it, and check whether the abort was
// performed correctly.
void Test(connection_base &C, bool ExplicitAbort)
{
  items<> BoringTuple(to_string(BoringYear), "yawn");

  pair<int,int> EventCounts;

  // First run our doomed transaction.  This will refuse to run if an event
  // exists for our Boring Year.
  {
    // Begin a transaction acting on our current connection; we'll abort it
    // later though.
    work Doomed(C, "Doomed");

    // Verify that our Boring Year was not yet in the events table
    EventCounts = CountEvents(Doomed);

    PQXX_CHECK_EQUAL(
	EventCounts.second,
	0,
	"Can't run, boring year is already in table.");

    // Now let's try to introduce a tuple for our Boring Year
    {
      tablewriter W(Doomed, Table);

      PQXX_CHECK_EQUAL(W.name(), Table, "tablewriter's name changed.");

      PQXX_CHECK_EQUAL(
	W.generate(BoringTuple),
	separated_list("\t", BoringTuple),
	"tablewriter writes new tuple incorrectly.");

      W.push_back(BoringTuple);
    }

    const pair<int,int> Recount = CountEvents(Doomed);
    PQXX_CHECK_EQUAL(
	Recount.second,
	 1,
	 "Wrong # events for " + to_string(BoringYear));

    PQXX_CHECK_EQUAL(
	Recount.first,
	EventCounts.first + 1,
	"Number of events changed.");

    // Okay, we've added an entry but we don't really want to.  Abort it
    // explicitly if requested, or simply let the Transaction object "expire."
    if (ExplicitAbort) Doomed.abort();

    // If now explicit abort requested, Doomed Transaction still ends here
  }

  // Now check that we're back in the original state.  Note that this may go
  // wrong if somebody managed to change the table between our two
  // transactions.
  work Checkup(C, "Checkup");

  const pair<int,int> NewEvents = CountEvents(Checkup);
  PQXX_CHECK_EQUAL(
	NewEvents.first,
	EventCounts.first,
	"Number of events changed.  This may be due to a bug in libpqxx, "
	"or the test table was modified by some other process.");

  PQXX_CHECK_EQUAL(
	NewEvents.second,
	0,
	"Found unexpected events.  This may be due to a bug in libpqxx, "
	"or the test table was modified by some other process.");
}


void test_abort(transaction_base &t)
{
  test::create_pqxxevents(t);
  connection_base &c(t.conn());
  t.commit();
  Test(c, true);
  Test(c, false);
}
} // namespace


PQXX_REGISTER_TEST_T(test_abort, nontransaction)
