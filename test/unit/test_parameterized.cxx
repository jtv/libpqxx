#include "../test_helpers.hxx"

using namespace std;
using namespace pqxx;

namespace
{
void test_exec_params(transaction_base &trans)
{
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  result r = trans.parameterized("SELECT $1 + 1")(12).exec();
#include <pqxx/internal/ignore-deprecated-post.hxx>
  PQXX_CHECK_EQUAL(
	r[0][0].as<int>(),
	13,
	"Bad outcome from parameterized statement.");

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  r = trans.parameterized("SELECT $1 || 'bar'")("foo").exec();
#include <pqxx/internal/ignore-deprecated-post.hxx>
  PQXX_CHECK_EQUAL(
	r[0][0].as<string>(),
	"foobar",
	"Incorrect string result from parameterized statement.");
}
} // namespace

PQXX_REGISTER_TEST(test_exec_params)
