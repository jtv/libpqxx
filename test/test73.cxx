#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Test pipeline's handling of SQL syntax errors on a
// more exotic connection type.  Using nontransaction so the pipeline gets to
// trigger the setup of the real connection.
namespace
{
void test_073()
{
  asyncconnection conn;
  nontransaction tx{conn};
  pipeline P{tx, "pipe73"};

  // Ensure all queries are issued at once to make the test more interesting
  P.retain();

  std::cout << "Opened " << P.classname() << " " << P.name() << ": "
       << P.description()
       << std::endl;

  // The middle query should fail; the surrounding two should succeed
  const auto id_1 = P.insert("SELECT 1");
  const auto id_f = P.insert("DELIBERATE SYNTAX ERROR");
  const auto id_2 = P.insert("SELECT 2");

  // See that we can process the queries without stumbling over the error
  P.complete();

  // We should be able to get the first result, which preceeds the error
  std::cout << "Retrieving initial result..." << std::endl;
  const int res_1 = P.retrieve(id_1).at(0).at(0).as<int>();
  std::cout << " - result was " << res_1 << std::endl;
  PQXX_CHECK_EQUAL(res_1, 1, "Got bad result from pipeline.");

  // We should *not* get a result for the query behind the error
  std::cout << "Retrieving post-error result..." << std::endl;
  quiet_errorhandler d{conn};
  PQXX_CHECK_THROWS(
	P.retrieve(id_2).at(0).at(0).as<int>(),
	std::runtime_error,
	"Pipeline wrongly resumed after SQL error.");

  // Now see that we get an exception when we touch the failed result
  std::cout << "Retrieving result for failed query..." << std::endl;
  PQXX_CHECK_THROWS(
	P.retrieve(id_f),
	sql_error,
	"Pipeline failed to register SQL error.");
}
} // namespace


PQXX_REGISTER_TEST(test_073);
