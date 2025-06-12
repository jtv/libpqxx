#include <iostream>

#include <pqxx/transaction>

#include "helpers.hxx"


namespace
{
// Simple test program for libpqxx.  Open connection to database, start
// a transaction, and perform a query inside it.
void test_001()
{
  pqxx::connection cx;

  // Begin a transaction acting on our current connection.  Give it a human-
  // readable name so the library can include it in error messages.
  pqxx::work tx{cx, "test1"};

  // Perform a query on the database, storing result rows in R.
  pqxx::result const r(tx.exec("SELECT * FROM pg_tables"));

  // We're expecting to find some tables...
  PQXX_CHECK(not std::empty(r), "No tables found.");

  tx.commit();
}


PQXX_REGISTER_TEST(test_001);
} // namespace
