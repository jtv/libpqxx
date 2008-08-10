#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_pipeline_detach(connection_base &, transaction_base &trans)
{
  // A pipeline grabs transaction focus, blocking regular queries and such.
  pipeline pipe(trans, "test_pipeline_detach");
  PQXX_CHECK_THROWS(
	trans.exec("SELECT 1"),
	logic_error,
	"Pipeline does not block regular queries");

  // Flushing a pipeline relinquishes transaction focus.
  pipe.flush();
  result r = trans.exec("SELECT 2");
  PQXX_CHECK_EQUAL(r.size(), 1u, "Wrong query result after flushing pipeline.");
  PQXX_CHECK_EQUAL(
    r[0][0].as<int>(),
    2,
    "Query returns wrong data after flushing pipeline.");

  // Inserting a query makes the pipeline grab transaction focus back.
  pipeline::query_id q = pipe.insert("SELECT 2");
  PQXX_CHECK_THROWS(
	trans.exec("SELECT 3"),
	logic_error,
	"Pipeline does not block regular queries");

  // Invoking complete() also detaches the pipeline from the transaction.
  pipe.complete();
  r = trans.exec("SELECT 4");
  PQXX_CHECK_EQUAL(r.size(), 1u, "Wrong query result after complete().");
  PQXX_CHECK_EQUAL(
    r[0][0].as<int>(),
    4,
    "Query returns wrong data after complete().");

  // The complete() also received any pending query results from the backend.
  r = pipe.retrieve(q);
  PQXX_CHECK_EQUAL(r.size(), 1u, "Wrong result from pipeline.");
  PQXX_CHECK_EQUAL(
    r[0][0].as<int>(),
    2,
    "Pipeline returned wrong data.");

}
} // namespace

int main()
{
  test::TestCase<> test("pipeline_detach", test_pipeline_detach);
  return test.run();
}

