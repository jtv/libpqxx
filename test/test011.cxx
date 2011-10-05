#include <cstdio>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Query a table and report its metadata.
namespace
{
void test_011(transaction_base &T)
{
  const string Table = "pg_tables";

  result R( T.exec("SELECT * FROM " + Table) );

  // Print column names
  for (tuple::size_type c = 0; c < R.columns(); ++c)
  {
    string N = R.column_name(c);
    cout << c << ":\t" << N << endl;
    PQXX_CHECK_EQUAL(R.column_number(N), c, "Inconsistent column numbers.");
  }

  // If there are rows in R, compare their metadata to R's.
  if (!R.empty())
  {
    PQXX_CHECK_EQUAL(R[0].rownumber(), 0u, "Row 0 has wrong number.");

    if (R.size() < 2)
      cout << "(Only one row in table.)" << endl;
    else
      PQXX_CHECK_EQUAL(R[1].rownumber(), 1u, "Row 1 has wrong number.");

    // Test tuple::swap()
    const tuple T1(R[0]), T2(R[1]);
    PQXX_CHECK_NOT_EQUAL(T1, T2, "Values are identical--can't test swap().");
    tuple T1s(T1), T2s(T2);
    PQXX_CHECK_EQUAL(T1s, T1, "Tuple copy-construction incorrect.");
    PQXX_CHECK_EQUAL(T2s, T2, "Tuple copy-construction inconsistently wrong.");
    T1s.swap(T2s);
    PQXX_CHECK_NOT_EQUAL(T1s, T1, "Tuple swap doesn't work.");
    PQXX_CHECK_NOT_EQUAL(T2s, T2, "Tuple swap inconsistently wrong.");
    PQXX_CHECK_EQUAL(T2s, T1, "Tuple swap is asymmetric.");
    PQXX_CHECK_EQUAL(T1s, T2, "Tuple swap is inconsistently asymmetric.");

    for (tuple::size_type c = 0; c < R[0].size(); ++c)
    {
      string N = R.column_name(c);

      PQXX_CHECK_EQUAL(
	string(R[0].at(c).c_str()),
	R[0].at(N).c_str(),
	"Field by name != field by number.");

      PQXX_CHECK_EQUAL(
	string(R[0][c].c_str()),
	R[0][N].c_str(),
	"at() is inconsistent with operator[].");

      PQXX_CHECK_EQUAL(R[0][c].name(), N, "Field names are inconsistent.");

      PQXX_CHECK_EQUAL(
	size_t(R[0][c].size()),
	strlen(R[0][c].c_str()),
	"Field size is not what we expected.");
    }
  }
  else
  {
    cout << "(Table is empty.)" << endl;
  }
}
} // namespace

PQXX_REGISTER_TEST(test_011)
