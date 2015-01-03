#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open a lazy connection to database, start
// a transaction, and perform a query inside it.
namespace
{
void test_021(transaction_base &)
{
  lazyconnection C;
  C.process_notice("Printing details on deferred connection\n");
  const string HostName = (C.hostname() ? C.hostname() : "<local>");
  C.process_notice(string() +
		   "database=" + C.dbname() + ", "
		   "username=" + C.username() + ", "
		   "hostname=" + HostName + ", "
		   "port=" + to_string(C.port()) + ", "
		   "options='" + C.options() + "', "
		   "backendpid=" + to_string(C.backendpid()) + "\n");

  work T(C, "test_021");

  // By now our connection should really have been created
  C.process_notice("Printing details on actual connection\n");
  C.process_notice(string() +
		   "database=" + C.dbname() + ", "
		   "username=" + C.username() + ", "
		   "hostname=" + HostName + ", "
		   "port=" + to_string(C.port()) + ", "
		   "options='" + C.options() + "', "
		   "backendpid=" + to_string(C.backendpid()) + "\n");

  string P;
  from_string(C.port(), P);
  PQXX_CHECK_EQUAL(P, to_string(C.port()), "Port string conversion is broken.");
  PQXX_CHECK_EQUAL(to_string(P), P, "Port string conversion is broken.");

  result R( T.exec("SELECT * FROM pg_tables") );

  T.process_notice(to_string(R.size()) + " "
		   "result row in transaction " +
		   T.name() +
		   "\n");

  // Process each successive result row
  for (result::const_iterator c = R.begin(); c != R.end(); ++c)
  {
    string N;
    c[0].to(N);

    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_021)
