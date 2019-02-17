#include "../test_helpers.hxx"

#include <pqxx/util>

using namespace pqxx;

namespace
{
void test_thread_safety_model()
{
  const thread_safety_model model = describe_thread_safety();

  if (model.safe_libpq and model.safe_kerberos)
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


PQXX_REGISTER_TEST(test_thread_safety_model);
} // namespace
