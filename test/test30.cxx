#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Query a table and report its metadata.
namespace
{
void test_030(pqxx::test::context &)
{
  std::string const table{"pg_tables"};

  pqxx::connection cx;
  pqxx::work tx{cx, "test30"};

  pqxx::result const R{tx.exec("SELECT * FROM " + table)};
  PQXX_CHECK(not std::empty(R), "Table " + table + " is empty, cannot test.");

  // Print column names
  for (pqxx::row::size_type c{0}; c < R.columns(); ++c)
  {
    std::string const N{R.column_name(c)};

    PQXX_CHECK_EQUAL(R[0].column_number(N), R.column_number(N));

    PQXX_CHECK_EQUAL(R[0].column_number(N), c);
  }

  // If there are rows in R, compare their metadata to R's.
  PQXX_CHECK_GREATER(
    std::size(R), 1,
    std::format("{} didn't have enough data for test.", table));

  PQXX_CHECK_EQUAL(R[0].row_number(), 0);
  PQXX_CHECK_EQUAL(R[1].row_number(), 1);

  for (pqxx::row::size_type c{0}; c < std::size(R[0]); ++c)
  {
    std::string const N{R.column_name(c)};

    PQXX_CHECK_EQUAL(std::string{R[0].at(c).c_str()}, R[0].at(N).c_str());

    PQXX_CHECK_EQUAL(std::string{R[0][c].c_str()}, R[0][N].c_str());

    PQXX_CHECK_EQUAL(R[0][c].name(), N);

    PQXX_CHECK_EQUAL(std::size(R[0][c]), std::strlen(R[0][c].c_str()));
  }
}


PQXX_REGISTER_TEST(test_030);
} // namespace
