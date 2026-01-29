#include <pqxx/transaction>

#include "helpers.hxx"


namespace
{
// Simple test program and example for libpqxx.  Open a connection to the
// database, start a transaction, and perform a query inside it.
void test_001(pqxx::test::context &)
{
  // Creating a pqxx::connection is the same thing as connecting to a database.
  // We can pass an optional connection string with connection options.  Our
  // tests however rely on the database being accessible with the default
  // parameters.
  //
  // If the connection should fail, this will throw an exception.  There's no
  // need to check whether the connection succeeded.
  pqxx::connection cx{""};

  // Begin a transaction acting on our current connection.  Give it a human-
  // readable name so that if there's an error, libpqxx can include that name
  // in the error message to help you debug.
  pqxx::work tx{cx, "test1"};

  // Perform a query on the database, storing result rows in r.
  pqxx::result const r(tx.exec("SELECT 42, 84"));

  // We got the one row that we selected.  The result object works a lot like a
  // normal C++ container.
  PQXX_CHECK(not std::empty(r));
  PQXX_CHECK_EQUAL(std::size(r), 1);

  // A result is two-dimensional though: it's got rows and columns.
  PQXX_CHECK_EQUAL(r.columns(), 2);

  // Each row acts pretty much as a container of fields.  (A field is the
  // intersection of one row and one column.)
  PQXX_CHECK_EQUAL(r[0][0].view(), "42");

  // To make our transaction take effect, we need to commit it.  If we don't
  // go through this, the transaction will roll back when the work object gets
  // destroyed.
  //
  // (Of course we made no changes to the database here, so in this case
  // there's nothing to commit and we might as well leave this out.)
  tx.commit();
}


PQXX_REGISTER_TEST(test_001);
} // namespace
