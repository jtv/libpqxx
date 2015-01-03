#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.

namespace
{
void test_014(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  // Begin a "non-transaction" acting on our current connection.  This is
  // really all the transactional integrity we need since we're only
  // performing one query which does not modify the database.
  nontransaction T(C, "test14");

  // The Transaction family of classes also has ProcessNotice() functions.
  // These simply pass the notice through to their connection, but this may
  // be more convenient in some cases.  All ProcessNotice() functions accept
  // C++ strings as well as C strings.
  T.process_notice(string("Started nontransaction\n"));

  result R( T.exec("SELECT * FROM pg_tables") );

  // Give some feedback to the test program's user prior to the real work
  T.process_notice(to_string(R.size()) + " "
	"result rows in transaction " +
	T.name() + "\n");

  for (result::const_iterator c = R.begin(); c != R.end(); ++c)
  {
    string N;
    c[0].to(N);

    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  // "Commit" the non-transaction.  This doesn't really do anything since
  // NonTransaction doesn't start a backend transaction.
  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_014, nontransaction)
