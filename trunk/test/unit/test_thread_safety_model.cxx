#include <test_helpers.hxx>

#include <pqxx/util>

using namespace std;
using namespace pqxx;

namespace
{
void test_thread_safety_model(transaction_base &)
{
  const thread_safety_model model = describe_thread_safety();

  if (model.have_safe_strerror &&
      model.safe_libpq &&
      model.safe_query_cancel &&
      model.safe_result_copy &&
      model.safe_kerberos)
    PQXX_CHECK_EQUAL(
	model.description,
	"",
	"Thread-safety looks okay but model description is nonempty.");
  else
    PQXX_CHECK_NOT_EQUAL(
	model.description,
	"",
	"Thread-safety model is imperfect but lacks description.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_thread_safety_model)
