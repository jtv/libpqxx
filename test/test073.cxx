#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Test pipeline's handling of SQL syntax errors on a
// more exotic connection type.  Using nontransaction so the pipeline gets to
// trigger the setup of the real connection.
namespace
{
void test_073(transaction_base &W)
{
  pipeline P(W, "pipe73");

  // Ensure all queries are issued at once to make the test more interesting
  P.retain();

  cout << "Opened " << P.classname() << " " << P.name() << ": "
       << P.description()
       << endl;

  // The middle query should fail; the surrounding two should succeed
  const pipeline::query_id id_1 = P.insert("SELECT 1");
  const pipeline::query_id id_f = P.insert("DELIBERATE SYNTAX ERROR");
  const pipeline::query_id id_2 = P.insert("SELECT 2");

  // See that we can process the queries without stumbling over the error
  P.complete();

  // We should be able to get the first result, which preceeds the error
  cout << "Retrieving initial result..." << endl;
  const int res_1 = P.retrieve(id_1).at(0).at(0).as<int>();
  cout << " - result was " << res_1 << endl;
  PQXX_CHECK_EQUAL(res_1, 1, "Got bad result from pipeline.");

  // We should *not* get a result for the query behind the error
  cout << "Retrieving post-error result..." << endl;
  quiet_errorhandler d(W.conn());
  PQXX_CHECK_THROWS(
	P.retrieve(id_2).at(0).at(0).as<int>(),
	runtime_error,
	"Pipeline wrongly resumed after SQL error.");

  // Now see that we get an exception when we touch the failed result
  cout << "Retrieving result for failed query..." << endl;
  PQXX_CHECK_THROWS(
	P.retrieve(id_f),
	sql_error,
	"Pipeline failed to register SQL error.");
}
} // namespace

PQXX_REGISTER_TEST_CT(test_073, asyncconnection, nontransaction)
