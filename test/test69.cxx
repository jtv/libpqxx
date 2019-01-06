#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Issue a query repeatedly through a pipeline, and
// compare results.
namespace
{
void TestPipeline(pipeline &P, int numqueries)
{
  const string Q("SELECT 99");

  for (int i=numqueries; i; --i) P.insert(Q);

  PQXX_CHECK(
	(numqueries == 0) or not P.empty(),
	"pipeline::empty() is broken.");

  int res = 0;
  for (int i=numqueries; i; --i)
  {
    PQXX_CHECK(not P.empty(), "Got wrong number of queries from pipeline.");

    pair<pipeline::query_id, result> R = P.retrieve();

    cout << "Query #" << R.first << ": " << R.second.at(0).at(0) << endl;
    if (res)
      PQXX_CHECK_EQUAL(
	R.second[0][0].as<int>(),
	res,
	"Got unexpected result out of pipeline.");

    res = R.second[0][0].as<int>();
  }

  PQXX_CHECK(P.empty(), "Pipeline not empty after retrieval.");
}


void test_069()
{
  asyncconnection conn;
  work tx{conn};
  pipeline P(tx);
  PQXX_CHECK(P.empty(), "Pipeline is not empty initially.");
  for (int i=0; i<5; ++i) TestPipeline(P, i);
}


PQXX_REGISTER_TEST(test_069);
} // namespace
