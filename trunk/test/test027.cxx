#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Read a table using a tablereader, which may be
// faster than a conventional query, on a lazy connection.
namespace
{
void test_027(transaction_base &T)
{
  const string Table = "pqxxevents";

  vector<string> R, First;

  {
    // Set up a tablereader stream to read data from table pg_tables
    tablereader Stream(T, Table);

    // Read results into string vectors and print them
    for (int n=0; (Stream >> R); ++n)
    {
      // Keep the first row for later consistency check
      if (n == 0) First = R;

      cout << n << ":\t";
      for (vector<string>::const_iterator i = R.begin(); i != R.end(); ++i)
        cout << *i << '\t';
      cout << endl;
      R.clear();
    }
  }

  // Verify the contents we got for the first row
  if (!First.empty())
  {
    tablereader Verify(T, Table);
    string Line;

    PQXX_CHECK(
	Verify.get_raw_line(Line),
	"tablereader got rows on the first read, but not on the second.");

    cout << "First tuple was: " << endl << Line << endl;

    Verify.tokenize(Line, R);
    PQXX_CHECK_EQUAL(
	R,
	First,
	"Re-parsing the first tuple yields different results.");
  }
}
} // namespace

PQXX_REGISTER_TEST_C(test_027, lazyconnection)
