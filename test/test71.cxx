#include <map>

#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Issue queries through a pipeline, and retrieve
// results both in-order and out-of-order.
namespace
{
using Exp = std::map<pqxx::pipeline::query_id, int>;

template<typename PAIR> void checkresult(pqxx::pipeline &P, PAIR c)
{
  pqxx::result const r{P.retrieve(c.first)};
  int const val{r.at(0).at(0).as(int(0))};
  PQXX_CHECK_EQUAL(val, c.second);
}


void test_071(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::pipeline P(tx);

  // Keep expected result for every query we issue
  Exp values;

  // Insert queries returning various numbers.
  for (int i{1}; i < 10; ++i)
    values[P.insert("SELECT " + pqxx::to_string(i))] = i;

  // Retrieve results in query_id order, and compare to expected values
  for (auto const &c : values) checkresult(P, c);

  PQXX_CHECK(std::empty(P));

  values.clear();

  // Insert more queries returning various numbers
  P.retain(20);
  for (int i{100}; i > 90; --i)
    values[P.insert("SELECT " + pqxx::to_string(i))] = i;

  P.resume();

  // Retrieve results in reverse order
  for (auto c{std::rbegin(values)}; c != std::rend(values); ++c)
    checkresult(P, *c);

  values.clear();
  P.retain(10);
  // With this batch size we can send up to 10 queries to the server at once.
  // However the pipeline will try to issue queries as soon as they come in,
  // meaning that it may never actually fill up a whole batch.
  for (int i{1010}; i > 1000; --i)
    values[P.insert(std::format("SELECT {}", i))] = i;

  // See that all results are retrieved by complete()
  P.complete();
  for (auto const &c : values) PQXX_CHECK(P.is_finished(c.first));
}
} // namespace


PQXX_REGISTER_TEST(test_071);
