#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.

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
  tx.process_notice(string{"Started nontransaction\n"});

  result R( tx.exec("SELECT * FROM pg_tables") );

  // Give some feedback to the test program's user prior to the real work
  tx.process_notice(
	to_string(R.size()) + " result rows in transaction " +
	tx.name() + "\n");

  for (const auto &c: R)
  {
    string N;
    c[0].to(N);

    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  // "Commit" the non-transaction.  This doesn't really do anything since
  // NonTransaction doesn't start a backend transaction.
  tx.commit();
}
} // namespace


PQXX_REGISTER_TEST(test_014);
