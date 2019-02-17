#include <cassert>
#include <iostream>
#include <iterator>
#include <list>

#include "../test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Define and use prepared statements.

#define COMPARE_RESULTS(name, lhs, rhs) \
  PQXX_CHECK_EQUAL(	\
	rhs,		\
	lhs, 		\
	"Executing " name " as prepared statement yields different results.");

namespace
{
std::string stringize(transaction_base &t, const std::string &arg)
{
  return "'" + t.esc(arg) + "'";
}


template<typename T> std::string stringize(transaction_base &t, T i)
{
  return stringize(t, to_string(i));
}


// Substitute variables in raw query.  This is not likely to be very robust,
// but it should do for just this test.  The main shortcomings are escaping,
// and not knowing when to quote the variables.
// Note we do the replacement backwards (meaning forward_only iterators won't
// do!) to avoid substituting e.g. "$12" as "$1" first.
template<typename ITER> std::string subst(
	transaction_base &t,
	std::string q,
	ITER patbegin,
	ITER patend)
{
  ptrdiff_t i = distance(patbegin, patend);
  for (ITER arg = patend; i > 0; --i)
  {
   --arg;
    const std::string marker = "$" + to_string(i),
	  var = stringize(t, *arg);
    const std::string::size_type msz = marker.size();
    while (q.find(marker) != std::string::npos)
	q.replace(q.find(marker),msz,var);
  }
  return q;
}


template<typename CNTNR> std::string subst(
	transaction_base &t,
	std::string q,
	const CNTNR &patterns)
{
  return subst(t, q, patterns.begin(), patterns.end());
}


#include <pqxx/internal/ignore-deprecated-pre.hxx>


/// @deprecated To be replaced with the C++11-style exec_prepared.
void test_legacy_prepared_statement()
{
  connection conn;
  work tx{conn};

  PQXX_CHECK(
	not tx.prepared("CountToTen").exists(),
	"Nonexistent prepared statement thinks it exists.");

  // Prepare a simple statement.
  conn.prepare("CountToTen", "SELECT * FROM generate_series(1, 10)");

  PQXX_CHECK(
	tx.prepared("CountToTen").exists(),
	"Prepared statement thinks it doesn't exist.");

  // See if a basic prepared statement works just like a regular query.
  PQXX_CHECK_EQUAL(
	tx.prepared("CountToTen").exec(),
	tx.exec("SELECT * FROM generate_series(1, 10)"),
	"CountToTen");

  // Try prepare_now() on an already prepared statement.
  conn.prepare_now("CountToTen");

  // Drop prepared statement.
  conn.unprepare("CountToTen");

  PQXX_CHECK_THROWS_EXCEPTION(
	conn.prepare_now("CountToTen"),
	"prepare_now() succeeded on dropped statement.");

  // It's okay to unprepare a statement repeatedly.
  conn.unprepare("CountToTen");
  conn.unprepare("CountToTen");

  // Executing an unprepared statement fails.
  PQXX_CHECK_THROWS_EXCEPTION(
	tx.prepared("CountToTen").exec(),
	"Execute unprepared statement didn't fail.");

  // Once unprepared, a statement can be prepared and used again.
  conn.prepare(
	"CountToTen",
	"SELECT generate_series FROM generate_series(1, 10)");
  conn.prepare_now("CountToTen");
  COMPARE_RESULTS("CountToTen_2",
	tx.prepared("CountToTen").exec(),
	tx.exec("SELECT * FROM generate_series(1, 10)"));

  // Double preparation of identical statement should be ignored...
  conn.prepare(
	"CountToTen",
	"SELECT generate_series FROM generate_series(1, 10)");
  COMPARE_RESULTS("CountToTen_double",
	tx.prepared("CountToTen").exec(),
	tx.exec("SELECT * FROM generate_series(1, 10)"));

  // ...But a modified definition shouldn't.
  PQXX_CHECK_THROWS_EXCEPTION(
	conn.prepare(
		"CountToTen",
		"SELECT generate_series FROM generate_series(1, 11)"),
	"Bad redefinition of statement went unnoticed.");

  // Test prepared statement with parameter.

  conn.prepare("CountUpToTen", "SELECT * FROM generate_series($1, 10)");

  std::vector<int> args = {2};
  COMPARE_RESULTS("CountUpToTen_seq",
	tx.prepared("CountUpToTen")(args[0]).exec(),
	tx.exec(subst(tx, "SELECT * FROM generate_series($1, 10)", args)));

  // Test prepared statement with 2 parameters.

  conn.prepare(
	"CountRange",
	"SELECT * FROM generate_series($1::int, $2::int)");
  COMPARE_RESULTS("CountRange_seq",
      tx.prepared("CountRange")(2)(5).exec(),
      tx.exec("SELECT * FROM generate_series(2, 5)"));

  // Test prepared statement with a null parameter.
  std::vector<const char *> ptrs = {nullptr, "99"};

  COMPARE_RESULTS("CountRange_null1",
	tx.prepared("CountRange")(ptrs[0])(ptrs[1]).exec(),
	tx.exec("SELECT * FROM generate_series(NULL, 99)"));

  // Test prepared statement with a binary parameter.
  conn.prepare("GimmeBinary", "SELECT $1::bytea");

  const binarystring bin_data{std::string{"x \x01 \x02 \xff y"}};

  PQXX_CHECK_EQUAL(
	binarystring(tx.prepared("GimmeBinary")(bin_data).exec()[0][0]).str(),
	bin_data.str(),
	"Binary parameter was mangled somewhere along the way.");

  const binarystring nully("x\0y", 3);
  PQXX_CHECK_EQUAL(
	binarystring(tx.prepared("GimmeBinary")(nully).exec()[0][0]).str(),
	nully.str(),
	"Binary string breaks on nul byte.");

  // Test unnamed prepared statement.
  conn.prepare("SELECT 2*$1");
  int outcome = tx.prepared()(9).exec()[0][0].as<int>();
  PQXX_CHECK_EQUAL(outcome, 18, "Unnamed prepared statement went mad.");

  // Redefine unnamed prepared statement.  Does not need to be unprepared
  // first.
  conn.prepare("SELECT 2*$1 + $2");
  outcome = tx.prepared()(9)(2).exec()[0][0].as<int>();
  PQXX_CHECK_EQUAL(outcome, 20, "Unnamed statement not properly redefined.");

  // Unlike how the unnamed prepared statement works in libpq, we can issue
  // other queries and then re-use the unnamed prepared statement.
  tx.exec("SELECT 12");
  outcome = tx.prepared()(3)(1).exec()[0][0].as<int>();
  PQXX_CHECK_EQUAL(outcome, 7, "Unnamed statement isn't what it was.");
}


void test_registration_and_invocation(transaction_base &T)
{
  constexpr auto count_to_5 = "SELECT * FROM generate_series(1, 5)";
  PQXX_CHECK(
	not T.prepared("CountToFive").exists(),
	"Nonexistent prepared statement thinks it exists.");

  // Prepare a simple statement.
  T.conn().prepare("CountToFive", count_to_5);

  PQXX_CHECK(
	T.prepared("CountToFive").exists(),
	"Prepared statement thinks it doesn't exist.");

  // The statement returns exactly what you'd expect.
  COMPARE_RESULTS(
	"CountToFive",
	T.exec_prepared("CountToFive"),
	T.exec(count_to_5));

  // It's OK to re-prepare the same statement.
  T.conn().prepare("CountToFive", count_to_5);

  // Results are still the same.
  COMPARE_RESULTS(
	"CountToFive",
	T.exec_prepared("CountToFive"),
	T.exec(count_to_5));

  // But re-preparing it with a different definition is an error.
  PQXX_CHECK_THROWS(
	T.conn().prepare("CountToFive", "SELECT 5"),
	pqxx::argument_error,
	"Did not report conflicting re-definition of prepared statement.");

  // Executing a nonexistent prepared statement is also an error.
  PQXX_CHECK_THROWS(
	T.exec_prepared("NonexistentStatement"),
	pqxx::argument_error,
	"Did not report invocation of nonexistent prepared statement.");
}


void test_basic_args(transaction_base &T)
{
  T.conn().prepare("EchoNum", "SELECT $1::int");
  auto r = T.exec_prepared("EchoNum", 7);
  PQXX_CHECK_EQUAL(r.size(), 1u, "Did not get 1 row from prepared statement.");
  PQXX_CHECK_EQUAL(r.front().size(), 1u, "Did not get exactly one column.");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 7, "Got wrong result.");

  auto rw = T.exec_prepared1("EchoNum", 8);
  PQXX_CHECK_EQUAL(rw.size(), 1u, "Did not get 1 column from exec_prepared1.");
  PQXX_CHECK_EQUAL(rw[0].as<int>(), 8, "Got wrong result.");
}


void test_multiple_params(transaction_base &T)
{
  T.conn().prepare(
	"CountSeries",
	"SELECT * FROM generate_series($1::int, $2::int)");
  auto r = T.exec_prepared_n(4, "CountSeries", 7, 10);
  PQXX_CHECK_EQUAL(r.size(), 4u, "Wrong number of rows, but no error raised.");
  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 7, "Wrong $1.");
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 10, "Wrong $2.");

  T.conn().prepare(
	"Reversed",
	"SELECT * FROM generate_series($2::int, $1::int)");
  r = T.exec_prepared_n(3, "Reversed", 8, 6);
  PQXX_CHECK_EQUAL(
	r.front().front().as<int>(),
	6,
	"Did parameters get reordered?");
  PQXX_CHECK_EQUAL(
	r.back().front().as<int>(),
	8,
	"$2 did not come through properly.");
}


void test_nulls(transaction_base &T)
{
  T.conn().prepare("EchoStr", "SELECT $1::varchar");
  auto rw = T.exec_prepared1("EchoStr", nullptr);
  PQXX_CHECK(rw.front().is_null(), "nullptr did not translate to null.");

  const char *n = nullptr;
  rw = T.exec_prepared1("EchoStr", n);
  PQXX_CHECK(rw.front().is_null(), "Null pointer did not translate to null.");
}


void test_strings(transaction_base &T)
{
  T.conn().prepare("EchoStr", "SELECT $1::varchar");
  auto rw = T.exec_prepared1("EchoStr", "foo");
  PQXX_CHECK_EQUAL(rw.front().as<std::string>(), "foo", "Wrong string result.");

  const char nasty_string[] = "'\\\"\\";
  rw = T.exec_prepared1("EchoStr", nasty_string);
  PQXX_CHECK_EQUAL(
	rw.front().as<std::string>(),
	std::string(nasty_string),
	"Prepared statement did not quote/escape correctly.");

  rw = T.exec_prepared1("EchoStr", std::string{nasty_string});
  PQXX_CHECK_EQUAL(
	rw.front().as<std::string>(),
	std::string(nasty_string),
	"Quoting/escaping went wrong in std::string.");

  char nonconst[] = "non-const C string";
  rw = T.exec_prepared1("EchoStr", nonconst);
  PQXX_CHECK_EQUAL(
	rw.front().as<std::string>(),
	std::string(nonconst),
	"Non-const C string passed incorrectly.");
}


void test_binary(transaction_base &T)
{
  T.conn().prepare("EchoBin", "SELECT $1::bytea");
  const char raw_bytes[] = "Binary\0bytes'\"with\tweird\xff bytes";
  const std::string input{raw_bytes, sizeof(raw_bytes)};
  const binarystring bin{input};

  auto rw = T.exec_prepared1("EchoBin", bin);
  PQXX_CHECK_EQUAL(
        binarystring(rw.front()).str(),
        input,
        "Binary string came out damaged.");
}


void test_dynamic_params(transaction_base &T)
{
  T.conn().prepare("Concat2Numbers", "SELECT 10 * $1 + $2");
  const std::vector<int> values{3, 9};
  const auto params = prepare::make_dynamic_params(values);
  const auto rw39 = T.exec_prepared1("Concat2Numbers", params);
  PQXX_CHECK_EQUAL(
        rw39.front().as<int>(),
        39,
        "Dynamic prepared-statement parameters went wrong.");

  T.conn().prepare("Concat4Numbers", "SELECT 1000*$1 + 100*$2 + 10*$3 + $4");
  const auto rw1396 = T.exec_prepared1("Concat4Numbers", 1, params, 6);
  PQXX_CHECK_EQUAL(
        rw1396.front().as<int>(),
        1396,
        "Dynamic params did not interleave with static ones properly.");
}


/// Test against any optional type, such as std::optional<int> or 
/// std::experimental::optional<int>.
template<typename Opt>
void test_optional(transaction_base &T)
{
  T.conn().prepare("EchoNum", "SELECT $1::int");
  pqxx::row rw = T.exec_prepared1(
    "EchoNum",
    pqxx::internal::make_optional<Opt>(10)
  );
  PQXX_CHECK_EQUAL(
	rw.front().as<int>(),
	10,
	"optional (with value) did not return the right value.");

  rw = T.exec_prepared1("EchoNum", Opt());
  PQXX_CHECK(
	rw.front().is_null(),
	"optional without value did not come out as null.");
}


void test_prepared_statements()
{
  connection conn;
  work tx{conn};
  test_registration_and_invocation(tx);
  test_basic_args(tx);
  test_multiple_params(tx);
  test_nulls(tx);
  test_strings(tx);
  test_binary(tx);
  test_dynamic_params(tx);

#if defined(PQXX_HAVE_OPTIONAL)
  test_optional<std::optional<int>>(tx);
#elif defined(PQXX_HAVE_EXP_OPTIONAL) && !defined(PQXX_HIDE_EXP_OPTIONAL)
  test_optional<std::experimental::optional<int>>(tx);
#endif
  test_optional<std::unique_ptr<int>>(tx);
  test_optional<std::shared_ptr<int>>(tx);
}


#include <pqxx/internal/ignore-deprecated-post.hxx>


void test_prepared_statement()
{
  test_legacy_prepared_statement();
  test_prepared_statements();
}


PQXX_REGISTER_TEST(test_prepared_statement);
} // namespace
