#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;
using namespace pqxx::test;


void empty() {}

void test_exception()
{
  PQXX_CHECK_THROWS(
	throw exception(),
	exception,
	"Plain exception not handled properly by PQXX_CHECK_THROWS.")
}

void test_simple_failure()
{
  throw test_failure(__FILE__, __LINE__, "(expected) Testing test failure.");
}

void test_check_throws_correct()
{
  PQXX_CHECK_THROWS(test_simple_failure(), test_failure,
	"PQXX_CHECK_THROWS() failed");
}

void test_check_throws_nothrow()
{
  PQXX_CHECK_THROWS(empty(), test_failure, "(expected)");
}

void test_check_throws_nothrow_meta()
{
  PQXX_CHECK_THROWS(test_check_throws_nothrow(), test_failure, "(expected)");
}

void test_check_throws_unexpected()
{
  PQXX_CHECK_THROWS(test_simple_failure(), int, "(expected)");
}

void test_check_throws_unexpected_meta()
{
  PQXX_CHECK_THROWS(test_check_throws_unexpected(), test_failure, "(expected)");
}


int main()
{
  int tr;

  // Check normal case: passing test.
  tr = pqxxtest(empty);
  if (tr != 0)
  {
    cerr << "Empty test failed: "
	"expected return value 0, got " << to_string(tr) << endl;
    return 1;
  }

  // Check that pqxxtest() handles and reports failures.
  tr = pqxxtest(test_simple_failure);
  if (tr != 1)
  {
    cerr << "Test for test failure failed: "
	"expected return value 1, got " << to_string(tr) << endl;
    return tr;
  }

  // Now we know that pqxxtest() works, use it to test PQXX_CHECK_THROWS().
  tr = pqxxtest(test_check_throws_correct);
  if (tr != 0)
  {
    cerr << "PQXX_CHECK_THROWS() failed 'correct' case: "
	"expected return value 0, got " << to_string(tr) << endl;
  }

  // Check that PQXX_CHECK_THROWS() fails if action does not throw.
  tr = pqxxtest(test_check_throws_nothrow);
  if (tr != 1)
  {
    cerr << "PQXX_CHECK_THROWS() failed 'no throw' case: "
	"expected return value 1, got " << to_string(tr) << endl;
  }

  // Check PQXX_CHECK_THROWS() on itself for case where action does not throw.
  tr = pqxxtest(test_check_throws_nothrow_meta);
  if (tr != 0)
  {
    cerr << "PQXX_CHECK_THROWS() failed to throw test_failure for 'no throw' "
	"case: expected return value 0, got " << to_string(tr) << endl;
  }

  // Check that PQXX_CHECK_THROWS() fails if action throws the wrong type.
  tr = pqxxtest(test_check_throws_unexpected);
  if (tr != 1)
  {
    cerr << "PQXX_CHECK_THROWS() failed 'unexpected' case: "
	"expected return value 1, got " << to_string(tr) << endl;
  }

  // Check PQXX_CHECK_THROWS() on itself for case where action does not throw.
  tr = pqxxtest(test_check_throws_unexpected_meta);
  if (tr != 0)
  {
    cerr << "PQXX_CHECK_THROWS() failed to throw test_failure for 'unexpected' "
	"case: expected return value 0, got " << to_string(tr) << endl;
  }

}

