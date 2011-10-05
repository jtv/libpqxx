#include <cstdio>
#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  See which fields in a query are null, and figure
// out whether any fields are lexicographically sorted.  Use lazy connection.
namespace
{
template<typename VEC, typename VAL>
void InitVector(VEC &V, typename VEC::size_type s, VAL val)
{
  V.resize(s);
  for (typename VEC::iterator i = V.begin(); i != V.end(); ++i) *i = val;
}


void test_031(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string Table = "pg_tables";

  // Tell C we won't be needing it for a while (not true, but let's pretend)
  C.deactivate();

  vector<int> NullFields;		// Maps column to no. of null fields
  vector<bool> SortedUp, SortedDown;	// Does column appear to be sorted?

  // Reactivate C (not really needed, but it sounds more polite)
  C.activate();

  work T(C, "test31");

  result R( T.exec("SELECT * FROM " + Table) );

  InitVector(NullFields, R.columns(), 0);
  InitVector(SortedUp, R.columns(), true);
  InitVector(SortedDown, R.columns(), true);

  for (result::const_iterator i = R.begin(); i != R.end(); i++)
  {
    PQXX_CHECK_EQUAL(
	(*i).rownumber(),
	i->rownumber(),
	"operator*() is inconsistent with operator->().");

    PQXX_CHECK_EQUAL(
	i->size(),
	R.columns(),
	"Row size is inconsistent with result::columns().");

    // Look for null fields
    for (tuple::size_type f=0; f<i->size(); ++f)
    {
      NullFields[f] += i.at(f).is_null();

      string A, B;
      PQXX_CHECK_EQUAL(
		i[f].to(A),
		i[f].to(B, string("")),
		"Variants of to() disagree on nullness.");

      PQXX_CHECK_EQUAL(A, B, "Variants of to() produce different values.");
    }

    // Compare fields to those of preceding row
    if (i != R.begin())
    {
      const result::const_iterator j = i - 1;

      // First perform some sanity checks on j vs. i and how libpqxx handles
      // their interrelationship...
      PQXX_CHECK_EQUAL(i - j, 1, "Iterator successor is at wrong distance.");

      PQXX_CHECK_NOT_EQUAL(j, i, "Iterator equals successor.");
      PQXX_CHECK(j != i, "Iterator is not different from successor.");
      PQXX_CHECK(!(j >= i), "Iterator does not precede successor.");
      PQXX_CHECK(!(j > i), "Iterator follows successor.");
      PQXX_CHECK(!(i <= j), "operator<=() is asymmetric.");
      PQXX_CHECK(!(i < j), "operator<() is asymmetric.");
      PQXX_CHECK(j <= i, "operator<=() is inconsistent.");
      PQXX_CHECK(j < i, "operator<() is inconsistent.");

      PQXX_CHECK_EQUAL( 1 + j, i, "Predecessor+1 brings us to wrong place.");

      result::const_iterator k(i);
      PQXX_CHECK_EQUAL(k--, i, "Post-decrement returns wrong value.");
      PQXX_CHECK_EQUAL(k, j, "Post-decrement goes to wrong position.");

      result::const_iterator l(i);
      PQXX_CHECK_EQUAL(--l, j, "Pre-decrement returns wrong value.");
      PQXX_CHECK_EQUAL(l, j, "Pre-decrement goes to wrong position.");

      PQXX_CHECK_EQUAL(k += 1, i, "operator+=() returns wrong value.");
      PQXX_CHECK_EQUAL(k, i, "operator+=() goes to wrong position.");

      PQXX_CHECK_EQUAL(k -= 1, j, "operator-=() returns wrong value.");
      PQXX_CHECK_EQUAL(k, j, "operator-=() goes to wrong position.");

      // ...Now let's do meaningful stuff with j, such as finding out which
      // fields may be sorted.  Don't do anything fancy like trying to
      // detect numbers and comparing them as such, just compare them as
      // simple strings.
      for (tuple::size_type f = 0; f < R.columns(); ++f)
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
  cout << "Read " << to_string(R.size()) << " rows." << endl;
  cout << "Field \t Field Name\t Nulls\t Sorted" << endl;

  for (tuple::size_type f = 0; f < R.columns(); ++f)
  {
    cout << to_string(f) << ":\t"
         << R.column_name(f) << '\t'
         << NullFields[f] << '\t'
         << (SortedUp[f] ?
		(SortedDown[f] ? "equal" : "up" ) :
		(SortedDown[f] ? "down" : "no" ) )
         << endl;

    PQXX_CHECK_BOUNDS(
	NullFields[f],
	0,
	int(R.size()) + 1,
	"Found more nulls than there were rows.");
  }
}
} // namespace

PQXX_REGISTER_TEST_CT(test_031, lazyconnection, nontransaction)
