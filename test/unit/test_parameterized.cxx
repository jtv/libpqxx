#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void test_exec_params()
{
  connection conn;
  work tx{conn};
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  result r = tx.parameterized("SELECT $1 + 1")(12).exec();
#include <pqxx/internal/ignore-deprecated-post.hxx>
  PQXX_CHECK_EQUAL(
	r[0][0].as<int>(),
	13,
	"Bad outcome from parameterized statement.");

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  r = tx.parameterized("SELECT $1 || 'bar'")("foo").exec();
#include <pqxx/internal/ignore-deprecated-post.hxx>
  PQXX_CHECK_EQUAL(
	r[0][0].as<std::string>(),
	"foobar",
	"Incorrect string result from parameterized statement.");
}


PQXX_REGISTER_TEST(test_exec_params);
} // namespace
