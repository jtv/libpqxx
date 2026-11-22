#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Test error handling for pipeline.
namespace
{
void test_072(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::pipeline P{tx};

  // Ensure all queries are issued at once to make the test more interesting
  P.retain();

  // The middle query should fail; the surrounding two should succeed
  auto const id_1{P.insert("SELECT 1")};
  auto const id_f{P.insert("SELECT * FROM pg_nonexist")};
  auto const id_2{P.insert("SELECT 2")};

  // See that we can process the queries without stumbling over the error
  P.complete();

  // We should be able to get the first result, which precedes the error
  auto const res_1{P.retrieve(id_1).at(0).at(0).as<int>()};
  PQXX_CHECK_EQUAL(res_1, 1);

  // We should *not* get a result for the query behind the error
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d{cx};
#include "pqxx/internal/ignore-deprecated-post.hxx"

    // For some reason when running in Valgrind this throws the error saying
    // that the transaction has been aborted due to an earlier error.
#if defined(PQXX_VALGRIND)
    PQXX_CHECK_THROWS(std::ignore = P.retrieve(id_2), pqxx::sql_error);
#else
    PQXX_CHECK_THROWS(std::ignore = P.retrieve(id_2), std::runtime_error);
#endif
  }

  // Now see that we get an exception when we touch the failed result
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d{cx};
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_THROWS(
      P.retrieve(id_f), pqxx::sql_error,
      "Pipeline failed to register SQL error.");
  }
}
} // namespace


PQXX_REGISTER_TEST(test_072);
