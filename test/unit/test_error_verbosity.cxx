#include "../test_helpers.hxx"

extern "C"
{
#include "libpq-fe.h"
}

using namespace pqxx;

namespace
{
void test_error_verbosity()
{
  PQXX_CHECK_EQUAL(
	int(error_verbosity::terse),
	int(PQERRORS_TERSE),
	"error_verbosity enum should match PGVerbosity.");
  PQXX_CHECK_EQUAL(
	int(error_verbosity::normal),
	int(PQERRORS_DEFAULT),
	"error_verbosity enum should match PGVerbosity.");
  PQXX_CHECK_EQUAL(
	int(error_verbosity::verbose),
	int(PQERRORS_VERBOSE),
	"error_verbosity enum should match PGVerbosity.");

  connection conn;
  PQXX_CHECK_EQUAL(
	int(conn.get_verbosity()),
	int(error_verbosity::normal),
	"Unexpected initial error verbosity.");

  conn.set_verbosity(error_verbosity::terse);

  PQXX_CHECK_EQUAL(
	int(conn.get_verbosity()),
	int(error_verbosity::terse),
	"Setting verbosity did not work.");
}


PQXX_REGISTER_TEST(test_error_verbosity);
} // namespace
