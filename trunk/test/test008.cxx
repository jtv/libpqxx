#include <iostream>
#include <stdexcept>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablereader>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Read a table using a tablereader, which
// may be faster than a conventional query.  A tablereader is really a frontend
// for a PostgreSQL COPY TO stdout command.

namespace
{
void test_008(connection_base &, transaction_base &T)
{
  const string Table = "pqxxevents";

  vector<string> R, First;

  // Set up a tablereader stream to read data from table pg_tables
  tablereader Stream(T, Table);

  // Read results into string vectors and print them
  for (int n=0; (Stream >> R); ++n)
  {
    // Keep the first row for later consistency check
    if (n == 0) First = R;

    cout << n << ":\t" << separated_list("\t",R.begin(),R.end()) << endl;
    R.clear();
  }

  Stream.complete();

  // Verify the contents we got for the first row
  if (!First.empty())
  {
    tablereader Verify(T, Table);
    string Line;

    const bool outcome(Verify.get_raw_line(Line));
    PQXX_CHECK(
	outcome,
	"tablereader got rows the first time around, but not the second time.");

    cout << "First tuple was: " << endl << Line << endl;

    Verify.tokenize(Line, R);
    PQXX_CHECK_EQUAL(R, First, "Got different results re-parsing first tuple.");
  }
}

} // namespace


int main()
{
  test::TestCase<> test008("test_008", test_008);
  return test::pqxxtest(test008);
}

