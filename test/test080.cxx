#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Read pqxxevents table using a tablereader, with a
// column list specification.  This requires a backend version of at least 7.3.
namespace
{
void test_080(transaction_base &T)
{
  const string Table = "pqxxevents";

  items<> Columns("year","event");
  items<> RevColumns;
  RevColumns("event")("year");

  vector<string> R, First;

  tablereader Stream(T, Table, Columns.begin(), Columns.end());

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
  Stream.complete();

  // Verify the contents we got for the first row
  if (!First.empty())
  {
    tablereader Verify(T, Table, RevColumns.begin(), RevColumns.end());
    string Line;

    PQXX_CHECK(
	Verify.get_raw_line(Line),
	"tablereader got rows on first read, but not on the second.");

    cout << "First tuple was: " << endl << Line << endl;

    Verify.tokenize(Line, R);
    const vector<string> &Rconst(R);
    vector<string> S;
    for (vector<string>::const_reverse_iterator i = Rconst.rbegin();
	 i != Rconst.rend();
	 ++i)
      S.push_back(*i);

    PQXX_CHECK(S == First, "Different results re-parsing first tuple.");
  }
}
} // namespace

PQXX_REGISTER_TEST_T(test_080, nontransaction)
