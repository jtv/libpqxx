#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test inhibition of connection reactivation
namespace
{
void test_086(transaction_base &N1)
{
  connection_base &C(N1.conn());
  const string Query = "SELECT * from pg_tables";

  cout << "Some datum: " << N1.exec(Query)[0][0] << endl;
  N1.commit();

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  C.inhibit_reactivation(true);
  C.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  quiet_errorhandler d(C);
  {
    nontransaction N2(C, "test86N2");
    PQXX_CHECK_THROWS(
	N2.exec(Query),
	broken_connection,
	"Deactivated connection did not throw broken_connection on exec().");
  }

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  C.inhibit_reactivation(false);
#include <pqxx/internal/ignore-deprecated-post.hxx>
  work W(C, "test86W");
  W.exec(Query);
  W.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_086, nontransaction)
