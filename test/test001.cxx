#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


namespace
{

// Simple test program for libpqxx.  Open connection to database, start
// a transaction, and perform a query inside it.
void test_001(transaction_base &trans)
{
  cout << "Connected to database." << endl
       << "Backend version: " << trans.conn().server_version() << endl
       << "Protocol version: " << trans.conn().protocol_version() << endl;

  // Close old transaction.
  trans.abort();

  // Begin a transaction acting on our current connection.  Give it a human-
  // readable name so the library can include it in error messages.
  work T(trans.conn(), "test1");

  // Perform a query on the database, storing result tuples in R.
  result R( T.exec("SELECT * FROM pg_tables") );

  // We're expecting to find some tables...
  PQXX_CHECK(!R.empty(), "No tables found.  Cannot test.");

  // Process each successive result tuple
  for (result::const_iterator c = R.begin(); c != R.end(); ++c)
  {
    // Dump tuple number and column 0 value to cout.  Read the value using
    // as(), which converts the field to the same type as the default value
    // you give it (or returns the default value if the field is null).
    cout << '\t' << to_string(c.num()) << '\t' << c[0].as(string()) << endl;
  }

  T.commit();
}

} // namespace

PQXX_REGISTER_TEST(test_001)
