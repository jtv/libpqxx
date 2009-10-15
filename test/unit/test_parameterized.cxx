#include "test_helpers.hxx"

#include "pqxx/config-internal-libpq.h"

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_exec_params(connection_base &, transaction_base &trans)
{
  if (!trans.conn().supports(connection_base::cap_parameterized_statements))
    return;

#ifdef PQXX_HAVE_PQEXECPARAMS
  result r = trans.parameterized("SELECT $1 + 1")(12).exec();
  PQXX_CHECK_EQUAL(
	r[0][0].as<int>(),
	13,
	"Bad outcome from parameterized statement.");

  r = trans.parameterized("SELECT $1 || 'bar'")("foo").exec();
  PQXX_CHECK_EQUAL(
	r[0][0].as<string>(),
	"foobar",
	"Incorrect string result from parameterized statement.");
#endif
}
} // namespace

PQXX_REGISTER_TEST(test_exec_params)
