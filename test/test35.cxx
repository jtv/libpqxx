#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
// This test combines a lazy connection with a robust transaction.
namespace
{
void test_035()
{
  lazyconnection conn;
  robusttransaction<> tx{conn};
  result R( tx.exec("SELECT * FROM pg_tables") );

  for (const auto &c: R)
  {
    std::string N;
    c[0].to(N);
    std::cout << '\t' << to_string(c.num()) << '\t' << N << std::endl;
  }

  tx.commit();
}


PQXX_REGISTER_TEST(test_035);
} // namespace
