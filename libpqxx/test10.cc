#include <cstdio>
#include <iostream>

#include "pg_connection.h"
#include "pg_transaction.h"
#include "pg_result.h"

using namespace PGSTD;
using namespace Pg;


// Simple test program for libpqxx.  Open connection to database, start
// a transaction, abort it, and verify that it "never happened."
//
// Usage: test10 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The program will attempt to add an entry to a table called "events",
// with a key column called "year"--and then abort the change.


// Function to print database's warnings  to cerr
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

const string Table = "events";


// Count events in table
int CountEvents(Transaction &T)
{
  const string EventsQuery = "SELECT count(*) FROM " + Table;
  int Count = 0;

  Result R( T.Exec(EventsQuery.c_str()) );
  R[0][0].to(Count);

  return Count;
}


// Count events that happened in Boring Year
int CountBoringEvents(Transaction &T)
{
  const string BoringQuery = "SELECT count(*) FROM " + Table + " "
                             "WHERE year=" + ToString(BoringYear);
  int Count = 0;

  Result R( T.Exec(BoringQuery.c_str()) );
  R[0][0].to(Count);

  return Count;
}
}


// Try adding a record, then aborting it, and check whether the abort was
// performed correctly.
void Test(Connection &C, bool ExplicitAbort)
{
  int Events;	// Count total number of events

  // First run our doomed transaction.  This will refuse to run if an event
  // exists for our Boring Year.
  {
    // Begin a transaction acting on our current connection; we'll abort it
    // later though.
    Transaction Doomed(C, "Doomed");

    // Verify that our Boring Year was not yet in the events table
    Events = CountEvents(Doomed);

    if (CountBoringEvents(Doomed))
      throw runtime_error("Can't run, year " +
			  ToString(BoringYear) + " "
			  "is already in table " +
			  Table);

    // Now let's try to introduce a tuple for our Boring Year
    Doomed.Exec(("INSERT INTO " + Table + " VALUES (" +
		 ToString(BoringYear) + ", "
		 "'yawn')").c_str());

    const int Count = CountBoringEvents(Doomed);
    if (Count != 1)
      throw runtime_error("Expected to find one event for " +
			  ToString(BoringYear) + ", "
			  "found " + 
			  ToString(Count));

    const int NewEvents = CountEvents(Doomed);
    if (NewEvents != Events+1)
      throw runtime_error("Number of events changed from " + 
			  ToString(Events) + " "
			  "to " +
			  ToString(NewEvents) + "; "
			  "expected " +
			  ToString(Events + 1));

    // Okay, we've added an entry but we don't really want to.  Abort it
    // explicitly if requested, or simply let the Transaction object "expire."
    if (ExplicitAbort) Doomed.Abort();

    // If now explicit abort requested, Doomed Transaction still ends here
  }

  // Now check that we're back in the original state.  Note that this may go
  // wrong if somebody managed to change the table between our two 
  // transactions.
  Transaction Checkup(C, "Checkup");

  const int NewEvents = CountEvents(Checkup);
  if (NewEvents != Events)
    throw runtime_error("Number of events changed from " + 
		        ToString(Events) + " "
			"to " +
			ToString(NewEvents) + "; "
			"this may be due to a bug in libpqxx, or the table "
			"was modified by some other process.");

  const int NewBoringEvents = CountBoringEvents(Checkup);
  if (NewBoringEvents)
    throw runtime_error("Found " +
		        ToString(NewBoringEvents) + " "
			"events in " +
			ToString(BoringYear) + "; "
			"wasn't expecting any.  This may be due to a bug in "
			"libpqxx, or the table was modified by some other "
			"process.");
}


int main(int argc, char *argv[])
{
  try
  {
    Connection C(argv[1] ? argv[1] : "");
    C.SetNoticeProcessor(ReportWarning, 0);
    C.Trace(stdout);

    // Test abort semantics, both with explicit and implicit abort
    Test(C, true);
    Test(C, false);

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

