#include <cassert>
#include <iostream>

#include <pqxx/connection.h>
#include <pqxx/nontransaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Modify the database inside a NonTransaction, and
// verify that the change gets made regardless of whether the NonTransaction is 
// eventually committed or aborted.
//
// Usage: test20 [connect-string] [table]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The table argument is the table being modified.  This defaults to "events".
// It is assumed to consist of an integer key called year, and a string.
namespace
{
int BoringYear = 1977;

} // namespace

int main(int argc, char *argv[])
{
  try
  {
    Connection C(argv[1] ? argv[1] : "");

    const string Table = ((argc > 2) ? argv[2] : "events");

    // Begin a transaction acting on our current connection
    NonTransaction T1(C, "T1");

    // Verify our start condition before beginning: there must not be a 1977
    // record already.
    Result R( T1.Exec(("SELECT * FROM " + Table + " "
	               "WHERE year=" + ToString(BoringYear)).c_str()) );
    if (R.size() != 0)
      throw runtime_error("There is already a record for " + 
	                  ToString(BoringYear) + ". "
		          "Can't run test.");

    // (Not needed, but verify that clear() works on empty containers)
    R.clear();
    if (!R.empty())
      throw logic_error("Result non-empty after clear()!");

    // OK.  Having laid that worry to rest, add a record for 1977.
    T1.Exec(("INSERT INTO " + Table + " VALUES"
             "(" +
	     ToString(BoringYear) + ","
	     "'Yawn'"
	     ")").c_str());

    // Abort T1.  Since T1 is a NonTransaction, which provides only the
    // transaction class interface without providing any form of transactional
    // integrity, this is not going to undo our work.
    T1.Abort();

    // Verify that our record was added, despite the Abort()
    NonTransaction T2(C, "T2");
    R = T2.Exec(("SELECT * FROM " + Table + " "
		 "WHERE year=" + ToString(BoringYear)).c_str());
    if (R.size() != 1)
      throw runtime_error("Expected to find 1 record for " + 
		          ToString(BoringYear) + ", found " +
			  ToString(R.size()) + ". "
			  "This could be a bug in libpqxx, "
			  "or something else modified the table.");

    if (R.capacity() < R.size())
      throw logic_error("Result's capacity is too small!");

    R.clear();
    if (!R.empty())
      throw logic_error("Result::clear() doesn't work!");

    // Now remove our record again
    T2.Exec(("DELETE FROM " + Table + " "
	     "WHERE year=" + ToString(BoringYear)).c_str());

    T2.Commit();

    // And again, verify results
    NonTransaction T3(C, "T3");

    R = T3.Exec(("SELECT * FROM " + Table + " "
	         "WHERE year=" + ToString(BoringYear)).c_str());
    if (R.size() != 0)
      throw runtime_error("Expected record for " + ToString(BoringYear) + " "
		          "to be gone but found " + ToString(R.size()) + ". "
			  "This could be a bug in libpqxx, "
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

