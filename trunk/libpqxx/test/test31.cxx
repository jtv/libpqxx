#include <cstdio>
#include <iostream>
#include <vector>

#include <pqxx/connection.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  See which fields in a query are null, and figure
// out whether any fields are lexicographically sorted.  Use lazy connection.
//
// Usage: test31 [connect-string] [table]
//
// Where table is the table to be queried; if none is given, pg_tables is
// queried by default.
//
// The connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{
template<typename VEC, typename VAL> 
void InitVector(VEC &V, typename VEC::size_type s, VAL val)
{
  V.resize(s);
  for (typename VEC::iterator i = V.begin(); i != V.end(); ++i) *i = val;
}
} // namespace

int main(int argc, char *argv[])
{
  try
  {
    const string Table = ((argc >= 3) ? argv[2] : "pg_tables");

    Connection C(argv[1] ? argv[1] : "", false);
    Transaction T(C, "test31");

    Result R( T.Exec(("SELECT * FROM " + Table).c_str()) );

    vector<int> NullFields;		// Maps column to no. of null fields
    vector<bool> SortedUp, SortedDown;	// Does column appear to be sorted?

    InitVector(NullFields, R.Columns(), 0);
    InitVector(SortedUp, R.Columns(), true);
    InitVector(SortedDown, R.Columns(), true);

    for (Result::const_iterator i = R.begin(); i != R.end(); i++)
    {
      if ((*i).Row() != i->Row())
	throw logic_error("Inconsistent rows: operator*() says " + 
			  ToString((*i).Row()) + ", "
			  "operator->() says " +
			  ToString(i->Row()));

      if (i->size() != R.Columns())
	throw logic_error("Row claims to have " + ToString(i->size()) + " "
			  "fields, but result claims to have " +
			  ToString(R.Columns()) + " columns!");

      // Look for null fields
      for (Result::Tuple::size_type f=0; f<i->size(); ++f)
      {
	NullFields[f] += i.at(f).is_null();

        string A, B;
        if (i[f].to(A) != i[f].to(B, string("")))
          throw logic_error("Variants of to() disagree on nullness!");

        if (A != B)
          throw logic_error("Field is '" + A + "' according to one to(), "
			    "but '" + B + "' to the other!");
      }

      // Compare fields to those of preceding row
      if (i != R.begin())
      {
        const Result::const_iterator j = i - 1;

	// First perform some sanity checks on j vs. i and how libpqxx handles
	// their interrelationship...
	if ((i - j) != 1)
	  throw logic_error("Difference between iterator and successor is " +
			    ToString(j-i));

	if ((j == i) || !(j != i) || 
	    (j >= i) || (j > i) ||
	    (i <= j) || (i < j) ||
	    !(j <= i) || !(j < i))
          throw logic_error("Something wrong in comparison between iterator "
			    "and its successor!");

	if ((1 + j) != i)
	  throw logic_error("Adding iterator's predecessor to 1 "
			    "doesn't bring us back to original iterator!");

	Result::const_iterator k(i);
	if ((k-- != i) || (k != j))
          throw logic_error("Something wrong with increment operator!");

	Result::const_iterator l(i);
	if ((--l != j) || (l != j))
	  throw logic_error("Something wrong with pre-increment operator!");

	if ((k += 1) != i)
	  throw logic_error("Something wrong with += operator!");

	if ((k -= 1) != j)
	  throw logic_error("Something wrong with -= operator!");

	// ...Now let's do meaningful stuff with j, such as finding out which
	// fields may be sorted.  Don't do anything fancy like trying to
	// detect numbers and comparing them as such, just compare them as
	// simple strings.
	for (Result::Tuple::size_type f = 0; f < R.Columns(); ++f)
	{
	  if (!j[f].is_null())
	  {
	    const bool U = SortedUp[f],
	               D = SortedDown[f];

	    SortedUp[f] = U & (string(j[f].c_str()) <= string(i[f].c_str()));
	    SortedDown[f] = D & (string(j[f].c_str()) >= string(i[f].c_str()));
	  }
	}
      }
    }

    // Now report on what we've found
    cout << "Read " << ToString(R.size()) << " rows." << endl;
    cout << "Field \t Field Name\t Nulls\t Sorted" << endl;

    for (Result::Tuple::size_type f = 0; f < R.Columns(); ++f)
    {
      cout << ToString(f) << ":\t"
	   << R.ColumnName(f) << '\t'
	   << NullFields[f] << '\t'
	   << (SortedUp[f] ? 
		(SortedDown[f] ? "equal" : "up" ) : 
		(SortedDown[f] ? "down" : "no" ) )
	   << endl;

      if (NullFields[f] > int(R.size()))
	throw logic_error("Found more nulls than there were rows!");
    }
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

