#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test nontransaction.

namespace
{
void test_014()
{
  connection conn;

  // Begin a "non-transaction" acting on our current connection.  This is
  // really all the transactional integrity we need since we're only
  // performing one query which does not modify the database.
  nontransaction tx{conn, "test14"};

  // The transaction class family also has process_notice() functions.
  // These simply pass the notice through to their connection, but this may
  // be more convenient in some cases.  All process_notice() functions accept
  // C++ strings as well as C strings.
  tx.process_notice(std::string{"Started nontransaction\n"});

  result r{ tx.exec("SELECT * FROM pg_tables") };

  for (const auto &c: r)
  {
    std::string n;
    c[0].to(n);

    std::cout << '\t' << to_string(c.num()) << '\t' << n << std::endl;
  }

  // "Commit" the non-transaction.  This doesn't really do anything since
  // nontransaction doesn't start a backend transaction.
  tx.commit();
}


PQXX_REGISTER_TEST(test_014);
} // namespace
