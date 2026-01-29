#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Query a table and report its metadata.
namespace
{
void test_011(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  std::string const Table{"pg_tables"};

  pqxx::result const r(tx.exec("SELECT * FROM " + Table));

  // Print column names
  for (pqxx::row::size_type c{0}; c < r.columns(); ++c)
  {
    std::string const n{r.column_name(c)};
    PQXX_CHECK_EQUAL(r.column_number(n), c);
  }

  // If there are rows in r, compare their metadata to r's.
  if (not std::empty(r))
  {
    PQXX_CHECK_EQUAL(r[0].row_number(), 0);

    if (std::size(r) >= 2)
      PQXX_CHECK_EQUAL(r[1].row_number(), 1);

    // Test result::iterator::swap()
    pqxx::result::const_iterator const T1(r.begin() + 0), T2(r.begin() + 1);
    PQXX_CHECK_NOT_EQUAL(T1, T2, "Values are identical--can't test swap().");
    pqxx::result::const_iterator T1s(T1), T2s(T2);
    PQXX_CHECK_EQUAL(T1s, T1, "Result iterator copy-construction is wrong.");
    PQXX_CHECK_EQUAL(
      T2s, T2, "Result iterator copy-construction is inconsistently wrong.");
    T1s.swap(T2s);
    PQXX_CHECK_NOT_EQUAL(T1s, T1, "Result iterator swap doesn't work.");
    PQXX_CHECK_NOT_EQUAL(T2s, T2);
    PQXX_CHECK_EQUAL(T2s, T1);
    PQXX_CHECK_EQUAL(T1s, T2);

    for (pqxx::row::size_type c{0}; c < std::size(r[0]); ++c)
    {
      std::string const n{r.column_name(c)};

      PQXX_CHECK_EQUAL(std::string{r[0].at(c).c_str()}, r[0].at(n).c_str());

      PQXX_CHECK_EQUAL(std::string{r[0][c].c_str()}, r[0][n].c_str());

      PQXX_CHECK_EQUAL(r[0][c].name(), n);

      PQXX_CHECK_EQUAL(std::size(r[0][c]), strlen(r[0][c].c_str()));
    }
  }
}


PQXX_REGISTER_TEST(test_011);
} // namespace
