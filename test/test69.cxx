#include <iostream>

#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Issue a query repeatedly through a pipeline, and
// compare results.
namespace
{
void TestPipeline(pqxx::pipeline &P, int numqueries)
{
  std::string const Q{"SELECT 99"};

  for (int i{numqueries}; i > 0; --i) P.insert(Q);

  PQXX_CHECK((numqueries == 0) or not std::empty(P));

  int res{0};
  for (int i{numqueries}; i > 0; --i)
  {
    PQXX_CHECK(not std::empty(P));

    auto R{P.retrieve()};

    if (res != 0)
      PQXX_CHECK_EQUAL(R.second.one_field().as<int>(), res);

    res = R.second.one_field().as<int>();
  }

  PQXX_CHECK(std::empty(P));
}


void test_069()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::pipeline P(tx);
  PQXX_CHECK(std::empty(P));
  for (int i{0}; i < 5; ++i) TestPipeline(P, i);
}


PQXX_REGISTER_TEST(test_069);
} // namespace
