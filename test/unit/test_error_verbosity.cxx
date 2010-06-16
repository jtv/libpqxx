#include "test_helpers.hxx"

extern "C"
{
#include "libpq-fe.h"
}

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_error_verbosity(transaction_base &trans)
{
  PQXX_CHECK_EQUAL(
	int(connection_base::terse),
	int(PQERRORS_TERSE),
	"error_verbosity enum should match PGVerbosity.");
  PQXX_CHECK_EQUAL(
	int(connection_base::normal),
	int(PQERRORS_DEFAULT),
	"error_verbosity enum should match PGVerbosity.");
  PQXX_CHECK_EQUAL(
	int(connection_base::verbose),
	int(PQERRORS_VERBOSE),
	"error_verbosity enum should match PGVerbosity.");

  PQXX_CHECK_EQUAL(
	int(trans.conn().get_verbosity()),
	int(connection_base::normal),
	"Unexpected initial error verbosity.");

  trans.conn().set_verbosity(connection_base::terse);

  PQXX_CHECK_EQUAL(
	int(trans.conn().get_verbosity()),
	int(connection_base::terse),
	"Setting verbosity did not work.");
}
} // namespace

PQXX_REGISTER_TEST(test_error_verbosity)
