#include <cstdio>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  See which fields in a query are null, and figure
// out whether any fields are lexicographically sorted.

namespace
{
template<typename VEC, typename VAL>
void InitVector(VEC &V, typename VEC::size_type s, VAL val)
{
  V.resize(s);
  for (typename VEC::iterator i = V.begin(); i != V.end(); ++i) *i = val;
}

void test_012(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string Table = "pg_tables";

  // Tell C we won't be needing it for a while (not true, but let's pretend)
  C.deactivate();

  // Now set up some data structures
  vector<int> NullFields;		// Maps column to no. of null fields
  vector<bool> SortedUp, SortedDown;	// Does column appear to be sorted?

  // ...And reactivate C (not really needed, but it sounds more polite)
  C.activate();

  work T(C, "test12");

  result R( T.exec("SELECT * FROM " + Table) );

  InitVector(NullFields, R.columns(), 0);
  InitVector(SortedUp, R.columns(), true);
  InitVector(SortedDown, R.columns(), true);

  for (result::const_iterator i = R.begin(); i != R.end(); i++)
  {
    PQXX_CHECK_EQUAL(
	(*i).rownumber(),
	i->rownumber(),
	"Inconsistent row numbers for operator*() and operator->().");

    PQXX_CHECK_EQUAL(i->size(), R.columns(), "Inconsistent row size.");

    // Look for null fields
    for (tuple::size_type f=0; f<i->size(); ++f)
    {
      NullFields[f] += i.at(f).is_null();

      string A, B;
      PQXX_CHECK_EQUAL(
	i[f].to(A),
	i[f].to(B, string("")),
	"Variants of to() disagree on nullness.");

      PQXX_CHECK_EQUAL(A, B, "Inconsistent field contents.");
    }

    // Compare fields to those of preceding row
    if (i != R.begin())
    {
      const result::const_iterator j = i - 1;

      // First perform some sanity checks on j vs. i and how libpqxx handles
      // their interrelationship...
      PQXX_CHECK_EQUAL(i - j, 1, "Iterator is wrong distance from successor.");

      PQXX_CHECK(!(j == i), "Iterator equals its successor.");
      PQXX_CHECK(j != i, "Iterator inequality is inconsistent.");
      PQXX_CHECK(!(j >= i), "Iterator doesn't come before its successor.");
      PQXX_CHECK(!(j > i), "Iterator is preceded by its successor.");
      PQXX_CHECK(!(i <= j), "Iterator doesn't come after its predecessor.");
      PQXX_CHECK(!(i < j), "Iterator is succeded by its predecessor.");
      PQXX_CHECK(j <= i, "operator<=() doesn't mirror operator>=().");
      PQXX_CHECK(j < i, "operator<() doesn't mirror operator>().");

      PQXX_CHECK_EQUAL(1 + j, i, "Adding 1 doesn't reach successor.");

      result::const_iterator k(i);
      PQXX_CHECK_EQUAL(k--, i, "Post-increment returns wrong iterator.");
      PQXX_CHECK_EQUAL(k, j, "Bad iterator position after post-increment.");

      result::const_iterator l(i);
      PQXX_CHECK_EQUAL(--l, j, "Pre-increment returns wrong iterator.");
      PQXX_CHECK_EQUAL(l, j, "Pre-increment sets wrong iterator position.");

      PQXX_CHECK_EQUAL(k += 1, i, "Wrong return value from +=.");
      PQXX_CHECK_EQUAL(k, i, "Bad iterator position after +=.");

      PQXX_CHECK_EQUAL(k -= 1, j, "Wrong return value from -=.");
      PQXX_CHECK_EQUAL(k, j, "Bad iterator position after -=.");

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

  for (tuple::size_type f = 0; f < R.columns(); ++f)
    PQXX_CHECK(
	NullFields[f] <= int(R.size()),
	"Found more nulls than there were rows.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_012, nontransaction)
