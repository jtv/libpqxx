#include <chrono>

#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_pipeline_is_consistent(pqxx::test::context &tctx)
{
  auto const num_queries = tctx.make_num(10) + 1;
  auto const value = tctx.make_num();

  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::pipeline pipe{tx};

  PQXX_CHECK(std::empty(pipe));

  auto const query{std::format("SELECT {}", value)};
  for (int i{0}; i < num_queries; ++i) pipe.insert(query);

  for (int i{0}; i < num_queries; ++i)
  {
    PQXX_CHECK(not std::empty(pipe));
    auto res{pipe.retrieve()};
    PQXX_CHECK_EQUAL(res.second.one_field().as<int>(), value);
  }

  PQXX_CHECK(pipe.empty());
}


void test_pipeline(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  // A pipeline grabs transaction focus, blocking regular queries and such.
  pqxx::pipeline pipe(tx, "test_pipeline_detach");
  PQXX_CHECK_THROWS(tx.exec("SELECT 1"), pqxx::usage_error);

  // Flushing a pipeline relinquishes transaction focus.
  pipe.flush();
  auto r{tx.exec("SELECT 2")};
  PQXX_CHECK_EQUAL(std::size(r), 1);
  PQXX_CHECK_EQUAL(r.one_field().as<int>(), 2);

  // Inserting a query makes the pipeline grab transaction focus back.
  auto q{pipe.insert("SELECT 2")};
  PQXX_CHECK_THROWS(
    tx.exec("SELECT 3"), pqxx::usage_error,
    "Pipeline does not block regular queries");

  // Invoking complete() also detaches the pipeline from the transaction.
  pipe.complete();
  r = tx.exec("SELECT 4");
  PQXX_CHECK_EQUAL(std::size(r), 1);
  PQXX_CHECK_EQUAL(r.one_field().as<int>(), 4);

  // The complete() also received any pending query results from the backend.
  r = pipe.retrieve(q);
  PQXX_CHECK_EQUAL(std::size(r), 1);
  PQXX_CHECK_EQUAL(r.one_field().as<int>(), 2);

  // We can cancel while the pipe is empty, and things will still work.
  pipe.cancel();

  // Issue a query and cancel it.  Measure time to see that we don't really
  // wait.
  using clock = std::chrono::steady_clock;
  auto const start{clock::now()};
  pipe.retain(0);
  pipe.insert("pg_sleep(10)");
  pipe.cancel();
  auto const finish{clock::now()};
  auto const seconds{
    std::chrono::duration_cast<std::chrono::seconds>(finish - start).count()};
  PQXX_CHECK_LESS(seconds, 5, "Canceling a sleep took suspiciously long.");
}
} // namespace

PQXX_REGISTER_TEST(test_pipeline_is_consistent);
PQXX_REGISTER_TEST(test_pipeline);
