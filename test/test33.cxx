#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_033()
{
  lazyconnection conn;
  nontransaction tx{conn};
  result R( tx.exec("SELECT * FROM pg_tables") );

  for (const auto &c: R)
  {
    std::string N;
    c[0].to(N);

    std::cout << '\t' << to_string(c.num()) << '\t' << N << std::endl;
  }

  // "Commit" the non-transaction.  This doesn't really do anything since
  // nontransaction doesn't start a backend transaction.
  tx.commit();
}


PQXX_REGISTER_TEST(test_033);
} // namespace
