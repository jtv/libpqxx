#include <cstdio>
#include <iostream>
#include <vector>

#include <pqxx/connection.h>
#include <pqxx/tablewriter.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Open connection to database, start a transaction, 
// abort it, and verify that it "never happened."
//
// Usage: test010 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.

namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
const int BoringYear = 1977;

const string Table = "pqxxevents";


// Count events, and boring events, in table
pair<int,int> CountEvents(Transaction &T)
{
  const string EventsQuery = "SELECT count(*) FROM " + Table;
  const string BoringQuery = EventsQuery + 
		             " WHERE year=" + 
			     ToString(BoringYear);
  int EventsCount = 0,
      BoringCount = 0;

  result R( T.Exec(EventsQuery) );
  R.at(0).at(0).to(EventsCount);

  R = T.Exec(BoringQuery);
  R.at(0).at(0).to(BoringCount);

  return make_pair(EventsCount, BoringCount);
}



// Try adding a record, then aborting it, and check whether the abort was
// performed correctly.
void Test(connection_base &C, bool ExplicitAbort)
{
  vector<string> BoringTuple;
  BoringTuple.push_back(ToString(BoringYear));
  BoringTuple.push_back("yawn");

  pair<int,int> EventCounts;

  // First run our doomed transaction.  This will refuse to run if an event
  // exists for our Boring Year.
  {
    // Begin a transaction acting on our current connection; we'll abort it
    // later though.
    Transaction Doomed(C, "Doomed");

    // Verify that our Boring Year was not yet in the events table
    EventCounts = CountEvents(Doomed);

    if (EventCounts.second)
      throw runtime_error("Can't run, year " +
			  ToString(BoringYear) + " "
			  "is already in table " +
			  Table);

    // Now let's try to introduce a tuple for our Boring Year
    {
      tablewriter W(Doomed, Table);

      if (W.Name() != Table)
        throw logic_error("Set tablewriter name to '" + Table + "', "
                "but now it's '" + W.Name() + "'");

      const string Literal = W.ezinekoT(BoringTuple);
      const string Expected = ToString(BoringYear) + "\t" + BoringTuple[1];
      if (Literal != Expected)
	throw logic_error("tablewriter writes new tuple as '" +
			  Literal + "', "
			  "ought to be '" +
			  Expected + "'");

      W.push_back(BoringTuple);
    }

    const pair<int,int> Recount = CountEvents(Doomed);
    if (Recount.second != 1)
      throw runtime_error("Expected to find one event for " +
			  ToString(BoringYear) + ", "
			  "found " + 
			  ToString(Recount.second));

    if (Recount.first != EventCounts.first+1)
      throw runtime_error("Number of events changed from " + 
			  ToString(EventCounts.first) + " "
			  "to " +
			  ToString(Recount.first) + "; "
			  "expected " +
			  ToString(EventCounts.first + 1));

    // Okay, we've added an entry but we don't really want to.  Abort it
    // explicitly if requested, or simply let the Transaction object "expire."
    if (ExplicitAbort) Doomed.Abort();

    // If now explicit abort requested, Doomed Transaction still ends here
  }

  // Now check that we're back in the original state.  Note that this may go
  // wrong if somebody managed to change the table between our two 
  // transactions.
  Transaction Checkup(C, "Checkup");

  const pair<int,int> NewEvents = CountEvents(Checkup);
  if (NewEvents.first != EventCounts.first)
    throw runtime_error("Number of events changed from " + 
		        ToString(EventCounts.first) + " "
			"to " +
			ToString(NewEvents.first) + "; "
			"this may be due to a bug in libpqxx, or the table "
			"was modified by some other process.");

  if (NewEvents.second)
    throw runtime_error("Found " +
		        ToString(NewEvents.second) + " "
			"events in " +
			ToString(BoringYear) + "; "
			"wasn't expecting any.  This may be due to a bug in "
			"libpqxx, or the table was modified by some other "
			"process.");
}

} // namespace


int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);

    // Test abort semantics, both with explicit and implicit abort
    Test(C, true);
    Test(C, false);

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


