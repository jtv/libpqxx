#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"


namespace
{
void TestPipeline(pqxx::pipeline &P, int numqueries)
{
  std::string const Q{"SELECT * FROM generate_series(1, 10)"};
  pqxx::result const Empty;
  PQXX_CHECK(std::empty(Empty));
  PQXX_CHECK(std::empty(Empty.query()));

  P.retain();
  for (int i{numqueries}; i > 0; --i) P.insert(Q);
  P.resume();

  PQXX_CHECK((numqueries == 0) || not std::empty(P));

  int res{0};
  pqxx::result Prev;
  PQXX_CHECK_EQUAL(Prev, Empty);

  for (int i{numqueries}; i > 0; --i)
  {
    PQXX_CHECK(not std::empty(P));

    auto R{P.retrieve()};

    PQXX_CHECK_NOT_EQUAL(R.second, Empty);
    if (Prev != Empty)
      PQXX_CHECK_NOT_EQUAL(R.second, Prev);

    Prev = R.second;
    PQXX_CHECK_EQUAL(Prev, R.second);
    PQXX_CHECK_EQUAL(R.second.query(), Q);

    if (res != 0)
      PQXX_CHECK_EQUAL(Prev[0][0].as<int>(), res);

    res = Prev[0][0].as<int>();
  }

  PQXX_CHECK(std::empty(P));
}


// Test program for libpqxx.  Issue a query repeatedly through a pipeline, and
// compare results.  Use retain() and resume() for performance.
void test_070(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::pipeline P(tx);

  PQXX_CHECK(std::empty(P));

  // Try to confuse the pipeline by feeding it a query and flushing
  P.retain();
  std::string const Q{"SELECT * FROM pg_tables"};
  P.insert(Q);
  P.flush();

  PQXX_CHECK(std::empty(P));

  // See if complete() breaks retain() as it should
  P.retain();
  P.insert(Q);
  PQXX_CHECK(not std::empty(P));
  P.complete();
  PQXX_CHECK(not std::empty(P));

  PQXX_CHECK_EQUAL(P.retrieve().second.query(), Q);

  PQXX_CHECK(std::empty(P));

  // See if retrieve() breaks retain() when it needs to
  P.retain();
  P.insert(Q);
  PQXX_CHECK_EQUAL(P.retrieve().second.query(), Q);

  // See if regular retain()/resume() works
  for (int i{0}; i < 5; ++i) TestPipeline(P, i);

    // See if retrieve() fails on an empty pipeline, as it should
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::quiet_errorhandler const d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_THROWS_EXCEPTION(P.retrieve());
}
} // namespace


PQXX_REGISTER_TEST(test_070);
