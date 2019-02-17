#include <cstdio>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Query a table and report its metadata.  Use lazy
// connection.
namespace
{
void test_030()
{
  const std::string Table = "pg_tables";

  lazyconnection conn;
  work tx{conn, "test30"};

  result R( tx.exec(("SELECT * FROM " + Table).c_str()) );
  PQXX_CHECK(not R.empty(), "Table " + Table + " is empty, cannot test.");

  // Print column names
  for (pqxx::row::size_type c = 0; c < R.columns(); ++c)
  {
    std::string N = R.column_name(c);
    std::cout << c << ":\t" << N << std::endl;

    PQXX_CHECK_EQUAL(
	R[0].column_number(N),
	R.column_number(N),
	"row::column_number() is inconsistent with result::column_number().");

    PQXX_CHECK_EQUAL(
	R[0].column_number(N.c_str()),
	c,
	"Inconsistent column numbers.");
  }

  // If there are rows in R, compare their metadata to R's.
  if (R.empty())
  {
    std::cout << "(Table is empty.)" << std::endl;
    return;
  }

  PQXX_CHECK_EQUAL(R[0].rownumber(), 0u, "Row 0 reports wrong number.");

  if (R.size() < 2)
    std::cout << "(Only one row in table.)" << std::endl;
  else
    PQXX_CHECK_EQUAL(R[1].rownumber(), 1u, "Row 1 reports wrong number.");

  for (pqxx::row::size_type c = 0; c < R[0].size(); ++c)
  {
    std::string N = R.column_name(c);

    PQXX_CHECK_EQUAL(
	std::string{R[0].at(c).c_str()},
	R[0].at(N).c_str(),
	"Different field values by name and by number.");

    PQXX_CHECK_EQUAL(
	std::string{R[0][c].c_str()},
	R[0][N].c_str(),
	"at() is inconsistent with operator[].");

    PQXX_CHECK_EQUAL(
	R[0][c].name(),
	N,
	"Inconsistent field names.");

    PQXX_CHECK_EQUAL(
	R[0][c].size(),
	std::strlen(R[0][c].c_str()),
	"Inconsistent field lengths.");
  }
}


PQXX_REGISTER_TEST(test_030);
} // namespace
