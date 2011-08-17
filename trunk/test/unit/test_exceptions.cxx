#include <test_helpers.hxx>

#include <pqxx/except>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_exceptions(transaction_base &)
{
  const string
    broken_query = "SELECT HORRIBLE ERROR",
    err = "Error message";

  try
  {
    throw sql_error(err, broken_query);
  }
  catch (const pqxx_exception &e)
  {
    PQXX_CHECK_EQUAL(e.base().what(), err, "Exception contains wrong message.");
    const sql_error *downcast = dynamic_cast<const sql_error *>(&e.base());
    PQXX_CHECK(downcast, "pqxx_exception-to-sql_error downcast is broken.");
    PQXX_CHECK_EQUAL(
	downcast->query(),
	broken_query,
	"Getting query from pqxx_exception is broken.");
  }
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_exceptions)
