#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


namespace
{

// Simple test program for libpqxx.  Open connection to database, start
// a transaction, and perform a query inside it.
void test_001()
{
  connection conn;
  std::cout
	<< "Connected to database." << std::endl
	<< "Backend version: " << conn.server_version() << std::endl
	<< "Protocol version: " << conn.protocol_version() << std::endl;

  // Begin a transaction acting on our current connection.  Give it a human-
  // readable name so the library can include it in error messages.
  work tx{conn, "test1"};

  // Perform a query on the database, storing result rows in R.
  result r( tx.exec("SELECT * FROM pg_tables") );

  // We're expecting to find some tables...
  PQXX_CHECK(not r.empty(), "No tables found.  Cannot test.");

  // Process each successive result row
  for (const auto &c: r)
  {
    // Dump row number and column 0 value to cout.  Read the value using
    // as(), which converts the field to the same type as the default value
    // you give it (or returns the default value if the field is null).
    std::cout
	<< '\t' << to_string(c.num()) << '\t' << c[0].as(std::string{})
	<< std::endl;
  }

  tx.commit();
}


PQXX_REGISTER_TEST(test_001);
} // namespace
