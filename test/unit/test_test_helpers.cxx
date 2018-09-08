#include "../test_helpers.hxx"

using namespace std;
using namespace pqxx;
using namespace pqxx::test;


namespace
{
void empty() {}


void test_check_notreached()
{
  // At a minimum, PQXX_CHECK_NOTREACHED must work.
  bool failed = true;
  try
  {
    PQXX_CHECK_NOTREACHED("(expected)");
    failed = false;
  }
  catch (const test_failure &)
  {
    // This is what we expect.
  }
  if (!failed)
    throw test_failure(__FILE__, __LINE__, "PQXX_CHECK_NOTREACHED is broken.");
}


// Test PQXX_CHECK.
void test_check()
{
  PQXX_CHECK(true, "PQXX_CHECK is broken.");

  bool failed = true;
  try
  {
    PQXX_CHECK(false, "(expected)");
    failed = false;
  }
  catch (const test_failure &)
  {
  }
  if (!failed) PQXX_CHECK_NOTREACHED("PQXX_CHECK failed to notice failure.");
}


// Test PQXX_CHECK_THROWS_EXCEPTION.
void test_check_throws_exception()
{
  // PQXX_CHECK_THROWS_EXCEPTION expects std::exception...
  PQXX_CHECK_THROWS_EXCEPTION(
	throw exception(),
	"PQXX_CHECK_THROWS_EXCEPTION did not catch std::exception.");

  // ...or any exception type derived from it.
  PQXX_CHECK_THROWS_EXCEPTION(
	throw test_failure(__FILE__, __LINE__, "(expected)"),
	"PQXX_CHECK_THROWS_EXCEPTION() failed to catch expected exception.");

  // Any other type is an error.
  bool failed = true;
  try
  {
    PQXX_CHECK_THROWS_EXCEPTION(throw 1, "(expected)");
    failed = false;
  }
  catch (const test_failure &)
  {
  }
  PQXX_CHECK(
	failed,
	"PQXX_CHECK_THROWS_EXCEPTION did not complain about non-exception.");

  // But there _must_ be an exception.
  failed = true;
  try
  {
    // If the test fails to throw, this throws a failure.
    PQXX_CHECK_THROWS_EXCEPTION(empty(), "(expected)");
    // So we shouldn't get to this point.
    failed = false;
  }
  catch (const test_failure &)
  {
    // Instead, we go straight here.
  }
  PQXX_CHECK(
	failed,
	"PQXX_CHECK_THROWS_EXCEPTION did not notice missing exception.");

  // PQXX_CHECK_THROWS_EXCEPTION can test itself...
  PQXX_CHECK_THROWS_EXCEPTION(
	PQXX_CHECK_THROWS_EXCEPTION(empty(), "(expected)"),
	"PQXX_CHECK_THROWS_EXCEPTION failed to throw for missing exception.");

  PQXX_CHECK_THROWS_EXCEPTION(
	PQXX_CHECK_THROWS_EXCEPTION(throw 1, "(expected)"),
	"PQXX_CHECK_THROWS_EXCEPTION ignored wrong exception type.");

}


// Test PQXX_CHECK_THROWS.
void test_check_throws()
{
  PQXX_CHECK_THROWS(
	throw test_failure(__FILE__, __LINE__, "(expected)"),
	 test_failure,
	"PQXX_CHECK_THROWS() failed to catch expected exception.");

  // Even if it's not std::exception-derived.
  PQXX_CHECK_THROWS(throw 1, int, "(expected)");

  // PQXX_CHECK_THROWS means there _must_ be an exception.
  bool failed = true;
  try
  {
    // If the test fails to throw, PQXX_CHECK_THROWS throws a failure.
    PQXX_CHECK_THROWS(empty(), std::runtime_error, "(expected)");
    // So we shouldn't get to this point.
    failed = false;
  }
  catch (const test_failure &)
  {
    // Instead, we go straight here.
  }
  PQXX_CHECK(failed, "PQXX_CHECK_THROWS did not notice missing exception.");

  // The exception must be of the right type (or a subclass of the right type).
  failed = true;
  try
  {
    // If the test throws the wrong type, PQXX_CHECK_THROWS throws a failure.
    PQXX_CHECK_THROWS(throw exception(), test_failure, "(expected)");
    failed = false;
  }
  catch (const test_failure &)
  {
    // Instead, we go straight here.
  }
  PQXX_CHECK(failed, "PQXX_CHECK_THROWS did not notice wrong exception type.");

  // PQXX_CHECK_THROWS can test itself...
  PQXX_CHECK_THROWS(
	PQXX_CHECK_THROWS(empty(), test_failure, "(expected)"),
	test_failure,
	"PQXX_CHECK_THROWS failed to throw for missing exception.");

  PQXX_CHECK_THROWS(
	PQXX_CHECK_THROWS(throw 1, std::runtime_error, "(expected)"),
	test_failure,
	"PQXX_CHECK_THROWS failed to throw for wrong exception type.");

}


void test_test_helpers(transaction_base &)
{
  test_check_notreached();
  test_check();
  test_check_throws_exception();
  test_check_throws();

  // Test other helpers against PQXX_CHECK_THROWS.
  PQXX_CHECK_THROWS(
	PQXX_CHECK_NOTREACHED("(expected)"),
	test_failure,
	"PQXX_CHECK_THROWS did not catch PQXX_CHECK_NOTREACHED.");

  PQXX_CHECK_THROWS(
	PQXX_CHECK(false, "(expected)"),
	test_failure,
	"PQXX_CHECK_THROWS did not catch failing PQXX_CHECK.");

  PQXX_CHECK_THROWS(
	PQXX_CHECK_THROWS(
		PQXX_CHECK(true, "(shouldn't happen)"),
		test_failure,
		"(expected)"),
	test_failure,
	"PQXX_CHECK_THROWS on successful PQXX_CHECK failed to throw.");

  // PQXX_CHECK_EQUAL tests for equality.  Its arguments need not be of the same
  // type, as long as equality between them is defined.
  PQXX_CHECK_EQUAL(1, 1, "PQXX_CHECK_EQUAL is broken.");
  PQXX_CHECK_EQUAL(1, 1L, "PQXX_CHECK_EQUAL breaks on type mismatch.");

  PQXX_CHECK_THROWS(
	PQXX_CHECK_EQUAL(1, 2, "(expected)"),
	test_failure,
	"PQXX_CHECK_EQUAL fails to spot inequality.");

  // PQXX_CHECK_NOT_EQUAL is like PQXX_CHECK_EQUAL, but tests for inequality.
  PQXX_CHECK_NOT_EQUAL(1, 2, "PQXX_CHECK_NOT_EQUAL is broken.");
  PQXX_CHECK_THROWS(
	PQXX_CHECK_NOT_EQUAL(1, 1, "(expected)"),
	test_failure,
	"PQXX_CHECK_NOT_EQUAL fails to fail when arguments are equal.");
  PQXX_CHECK_THROWS(
	PQXX_CHECK_NOT_EQUAL(1, 1L, "(expected)"),
	test_failure,
	"PQXX_CHECK_NOT_EQUAL breaks on type mismatch.");

  // PQXX_CHECK_BOUNDS checks a value against a range.
  PQXX_CHECK_BOUNDS(2, 1, 3, "PQXX_CHECK_BOUNDS wrongly finds fault.");

  PQXX_CHECK_THROWS(
	PQXX_CHECK_BOUNDS(1, 2, 3, "(Expected)"),
	test_failure,
	"PQXX_CHECK_BOUNDS did not detect value below permitted range.");

  // PQXX_CHECK_BOUNDS tests against a half-open interval.
  PQXX_CHECK_BOUNDS(1, 1, 3, "PQXX_CHECK_BOUNDS goes wrong on lower bound.");
  PQXX_CHECK_THROWS(
	PQXX_CHECK_BOUNDS(3, 1, 3, "(Expected)"),
	test_failure,
	"PQXX_CHECK_BOUNDS interval is not half-open.");

  // PQXX_CHECK_BOUNDS deals well with empty intervals.
  PQXX_CHECK_THROWS(
	PQXX_CHECK_BOUNDS(1, 2, 1, "(Expected)"),
	test_failure,
	"PQXX_CHECK_BOUNDS did not detect empty interval.");
}
} // namespace


PQXX_REGISTER_TEST_NODB(test_test_helpers)
