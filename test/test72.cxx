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

  // Enable batching of queries.
  P.retain(3);

  // One query in the middle of a pipeline fails.  First we insert a very slow
  // query, just so that the pipeline will combine the next queries into a
  // single batch.
  auto const id_0{P.insert("SELECT pg_sleep(2)")};
  auto const id_1{P.insert("SELECT 1")};
  auto const id_f{P.insert("SELECT * FROM pg_nonexist")};
  auto const id_2{P.insert("SELECT 2")};

  // See that we can process the queries without stumbling over the error
  P.complete();

  PQXX_CHECK_EQUAL(P.retrieve(id_0).size(), 1);

  // We have no trouble retrieving the result which precedes the error.
  auto const res_1{P.retrieve(id_1).at(0).at(0).as<int>()};
  PQXX_CHECK_EQUAL(res_1, 1);

  // We can't get a result for the query behind the error.  It effectively
  // never executed.
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d{cx};
#include "pqxx/internal/ignore-deprecated-post.hxx"

    PQXX_CHECK_THROWS(std::ignore = P.retrieve(id_2), std::runtime_error);
  }

  // The result for the failed query reflects the error.
  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d{cx};
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_THROWS(P.retrieve(id_f), pqxx::sql_error);
  }
}
} // namespace


PQXX_REGISTER_TEST(test_072);
