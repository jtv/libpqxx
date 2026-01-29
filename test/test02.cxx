#include <pqxx/transaction>

#include "helpers.hxx"


// Example/test program for libpqxx.  Perform a query and enumerate its output
// using array indexing.

namespace
{
/// Attempt to connect to a database, but fail.
void bad_connect()
{
  pqxx::connection const cx{"totally#invalid@connect$string!?"};
}

void test_002(pqxx::test::context &)
{
  // Before we really connect, test the expected behaviour of the default
  // connection type, where a failure to connect results in an immediate
  // exception rather than a silent retry.
  PQXX_CHECK_THROWS(
    bad_connect(), pqxx::broken_connection,
    "Invalid connection string did not cause exception.");

  // Actually connect to the database.  If we're happy to use the defaults
  // (in these tests we are) then we don't need to pass a connection string.
  pqxx::connection cx;

  // Start a transaction within the context of our connection.
  pqxx::work tx{cx, "test2"};

  // Perform a query within the transaction.
  pqxx::result const R(tx.exec("SELECT * FROM pg_tables"));

  // Let's keep the database waiting as briefly as possible: commit now,
  // before we start processing results.  We could do this later, or since
  // we're not making any changes in the database that need to be committed,
  // we could in this case even omit it altogether.
  tx.commit();

  // The result knows from which table each column originated.
  pqxx::oid const rtable{R.column_table(0)};
  PQXX_CHECK_EQUAL(
    rtable, R.column_table(pqxx::row::size_type(0)),
    "Inconsistent answers from column_table()");

  std::string const rcol{R.column_name(0)};
  pqxx::oid const crtable{R.column_table(rcol)};
  PQXX_CHECK_EQUAL(
    crtable, rtable, "Field looked up by name gives different origin.");

  // Now we've got all that settled, let's process our results.
  for (auto const &f : R)
  {
    pqxx::oid const ftable{f[0].table()};
    PQXX_CHECK_EQUAL(ftable, rtable, "field::table() is broken.");

    pqxx::oid const ttable{f.column_table(0)};

    PQXX_CHECK_EQUAL(
      ttable, f.column_table(pqxx::row::size_type(0)),
      "Inconsistent pqxx::row::column_table().");

    PQXX_CHECK_EQUAL(ttable, rtable, "Inconsistent result::column_table().");

    pqxx::oid const cttable{f.column_table(rcol)};

    PQXX_CHECK_EQUAL(cttable, rtable, "pqxx::row::column_table() is broken.");
  }
}


PQXX_REGISTER_TEST(test_002);
} // namespace
