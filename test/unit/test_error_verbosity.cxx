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
  work tx{conn};
  conn.set_verbosity(error_verbosity::terse);
  tx.exec1("SELECT 1");
  conn.set_verbosity(error_verbosity::verbose);
  tx.exec1("SELECT 2");
}


PQXX_REGISTER_TEST(test_error_verbosity);
} // namespace
