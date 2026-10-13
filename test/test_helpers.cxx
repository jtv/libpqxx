#include "helpers.hxx"

namespace
{
void empty() {}


// Test PQXX_CHECK.
void test_check(pqxx::test::context &)
{
  PQXX_CHECK(true);

  bool failed{true};
  try
  {
    // We expect this check to fail.
    PQXX_CHECK(false);
    failed = false;
  }
  catch (pqxx::test::test_failure const &)
  {}
  if (not failed)
    pqxx::test::check_notreached("PQXX_CHECK failed to notice failure.");
}


// Test PQXX_CHECK_THROWS_EXCEPTION.
void test_check_throws_exception(pqxx::test::context &)
{
  // PQXX_CHECK_THROWS_EXCEPTION expects std::exception...
  PQXX_CHECK_THROWS_EXCEPTION(
    throw std::exception(),
    "PQXX_CHECK_THROWS_EXCEPTION did not catch std::exception.");

  // ...or any exception type derived from it.
  PQXX_CHECK_THROWS_EXCEPTION(
    throw pqxx::test::test_failure{"(expected)"},
    "PQXX_CHECK_THROWS_EXCEPTION() failed to catch expected exception.");

  // Any other type is an error.
  bool failed{true};
  try
  {
    // NOLINTBEGIN(hicpp-exception-baseclass)
    PQXX_CHECK_THROWS_EXCEPTION(throw 1, "(expected)");
    // NOLINTEND(hicpp-exception-baseclass)
    failed = false;
  }
  catch (pqxx::test::test_failure const &)
  {}
  // We expect PQXX_CHECK_THROWS_EXCEPTION to fail when what's being thrown is
  // not a std::exception subclass.
  PQXX_CHECK(failed);

  // But there _must_ be an exception.
  failed = true;
  std::ignore = failed;
  try
  {
    // If the test fails to throw, this throws a failure.
    PQXX_CHECK_THROWS_EXCEPTION(empty(), "(expected)");
    // So we shouldn't get to this point.
    failed = false;
  }
  catch (pqxx::test::test_failure const &)
  {
    // Instead, we go straight here.
  }
  // PQXX_CHECK_THROWS_EXCEPTION fails if nothing was thrown.
  PQXX_CHECK(failed);

  // PQXX_CHECK_THROWS_EXCEPTION can test itself...
  PQXX_CHECK_THROWS_EXCEPTION(
    PQXX_CHECK_THROWS_EXCEPTION(empty(), "(expected)"),
    "PQXX_CHECK_THROWS_EXCEPTION failed to throw for missing exception.");

  // NOLINTBEGIN(hicpp-exception-baseclass)
  PQXX_CHECK_THROWS_EXCEPTION(
    PQXX_CHECK_THROWS_EXCEPTION(throw 1, "(expected)"),
    "PQXX_CHECK_THROWS_EXCEPTION ignored wrong exception type.");
  // NOLINTEND(hicpp-exception-baseclass)
}


// Test PQXX_CHECK_THROWS.
void test_check_throws(pqxx::test::context &)
{
  PQXX_CHECK_THROWS(
    throw pqxx::test::test_failure{"(expected)"}, pqxx::test::test_failure,
    "PQXX_CHECK_THROWS() failed to catch expected exception.");

  // Even if it's not std::exception-derived.

  // NOLINTBEGIN(hicpp-exception-baseclass)
  PQXX_CHECK_THROWS(throw 1, int, "(expected)");
  // NOLINTEND(hicpp-exception-baseclass)

  // PQXX_CHECK_THROWS means there _must_ be an exception.
  bool failed{true};
  try
  {
    // If the test fails to throw, PQXX_CHECK_THROWS throws a failure.
    PQXX_CHECK_THROWS(empty(), std::runtime_error, "(expected)");
    // So we shouldn't get to this point.
    failed = false;
  }
  catch (pqxx::test::test_failure const &)
  {
    // Instead, we go straight here.
  }
  // PQXX_CHECK_THROWS fails if nothing was thrown.
  PQXX_CHECK(failed);

  // The exception must be of the right type (or a subclass of the right type).
  failed = true;
  std::ignore = failed;
  try
  {
    // If the test throws the wrong type, PQXX_CHECK_THROWS throws a failure.
    PQXX_CHECK_THROWS(
      throw std::exception(), pqxx::test::test_failure, "(expected)");
    failed = false;
  }
  catch (pqxx::test::test_failure const &)
  {
    // Instead, we go straight here.
  }
  // PQXX_CHECK_THROWS fails if an exception was thrown but it's not the
  // expected type.
  PQXX_CHECK(failed);

  // PQXX_CHECK_THROWS can test itself...
  PQXX_CHECK_THROWS(
    PQXX_CHECK_THROWS(empty(), pqxx::test::test_failure, "(expected)"),
    pqxx::test::test_failure);

  // NOLINTBEGIN(hicpp-exception-baseclass)
  PQXX_CHECK_THROWS(
    PQXX_CHECK_THROWS(throw 1, std::runtime_error, "(expected)"),
    pqxx::test::test_failure);
  // NOLINTEND(hicpp-exception-baseclass)
}


void test_helpers(pqxx::test::context &)
{
  // Test other helpers against PQXX_CHECK_THROWS.

  // PQXX_CHECK_THROWS catches check_notreached().
  PQXX_CHECK_THROWS(
    pqxx::test::check_notreached("(expected)"), pqxx::test::test_failure);

  // PQXX_CHECK_THROWS catches a PQXX_CHECK failure.
  PQXX_CHECK_THROWS(PQXX_CHECK(false), pqxx::test::test_failure);

  // PQXX_CHECK_THROWS fails if no exception was thrown.
  PQXX_CHECK_THROWS(
    PQXX_CHECK_THROWS(PQXX_CHECK(true), pqxx::test::test_failure),
    pqxx::test::test_failure);

  // PQXX_CHECK_EQUAL tests for equality.  Its arguments need not be of the
  // same type, as long as equality between them is defined.
  PQXX_CHECK_EQUAL(1, 1);
  PQXX_CHECK_EQUAL(1, 1L);

  PQXX_CHECK_THROWS(PQXX_CHECK_EQUAL(1, 2), pqxx::test::test_failure);

  // PQXX_CHECK_NOT_EQUAL is like PQXX_CHECK_EQUAL, but tests for inequality.
  PQXX_CHECK_NOT_EQUAL(1, 2);
  PQXX_CHECK_THROWS(PQXX_CHECK_NOT_EQUAL(1, 1), pqxx::test::test_failure);
  PQXX_CHECK_THROWS(PQXX_CHECK_NOT_EQUAL(1, 1L), pqxx::test::test_failure);

  // PQXX_CHECK_BOUNDS checks a value against a range.
  PQXX_CHECK_BOUNDS(2, 1, 3);

  // PQXX_CHECK_BOUNDS catches values below the expected range.
  PQXX_CHECK_THROWS(PQXX_CHECK_BOUNDS(1, 2, 3), pqxx::test::test_failure);

  // PQXX_CHECK_BOUNDS tests against a half-open interval.
  PQXX_CHECK_BOUNDS(1, 1, 3);
  PQXX_CHECK_THROWS(PQXX_CHECK_BOUNDS(3, 1, 3), pqxx::test::test_failure);

  // PQXX_CHECK_BOUNDS deals well with empty intervals.
  PQXX_CHECK_THROWS(PQXX_CHECK_BOUNDS(1, 2, 1), pqxx::test::test_failure);
}


PQXX_REGISTER_TEST(test_helpers);
PQXX_REGISTER_TEST(test_check);
PQXX_REGISTER_TEST(test_check_throws_exception);
PQXX_REGISTER_TEST(test_check_throws);
} // namespace
