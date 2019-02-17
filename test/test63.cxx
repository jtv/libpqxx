#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx's asyncconnection.  Asynchronously open a
// connection to the database, start a transaction, and perform a query inside
// it.
namespace
{
void test_063()
{
  asyncconnection conn;
  work tx{conn};
  result R( tx.exec("SELECT * FROM pg_tables") );
  PQXX_CHECK(not R.empty(), "No tables found.  Cannot test.");

  for (const auto &c: R)
    std::cout
	<< '\t' << to_string(c.num()) << '\t' << c[0].as(std::string{})
	<< std::endl;

  tx.commit();
}


PQXX_REGISTER_TEST(test_063);
} // namespace
