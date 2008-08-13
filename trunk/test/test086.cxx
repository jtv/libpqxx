#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test inhibition of connection reactivation
namespace
{
void test_086(connection_base &C, transaction_base &N1)
{
  const string Query = "SELECT * from pg_tables";

  cout << "Some datum: " << N1.exec(Query)[0][0] << endl;
  N1.commit();

  C.inhibit_reactivation(true);
  C.deactivate();

  disable_noticer d(C);
  {
    nontransaction N2(C, "test86N2");
    PQXX_CHECK_THROWS(
	N2.exec(Query),
	broken_connection,
	"Deactivated connection did not throw broken_connection on exec().");
  }

  C.inhibit_reactivation(false);
  work W(C, "test86W");
  W.exec(Query);
  W.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_086, nontransaction)
