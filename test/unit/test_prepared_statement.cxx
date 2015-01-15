#include <cassert>
#include <iostream>
#include <list>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Define and use prepared statements.

#define COMPARE_RESULTS(name, lhs, rhs) \
  PQXX_CHECK_EQUAL(	\
	rhs,		\
	lhs, 		\
	"Executing " name " as prepared statement yields different results.");

namespace
{
string stringize(transaction_base &t, const string &arg)
{
  return "'" + t.esc(arg) + "'";
}


template<typename T> string stringize(transaction_base &t, T i)
{
  return stringize(t, to_string(i));
}


// Substitute variables in raw query.  This is not likely to be very robust,
// but it should do for just this test.  The main shortcomings are escaping,
// and not knowing when to quote the variables.
// Note we do the replacement backwards (meaning forward_only iterators won't
// do!) to avoid substituting e.g. "$12" as "$1" first.
template<typename ITER> string subst(transaction_base &t,
	string q,
	ITER patbegin,
	ITER patend)
{
  ptrdiff_t i = pqxx::internal::distance(patbegin, patend);
  for (ITER arg = patend; i > 0; --i)
  {
   --arg;
    const string marker = "$" + to_string(i),
	  var = stringize(t, *arg);
    const string::size_type msz = marker.size();
    while (q.find(marker) != string::npos) q.replace(q.find(marker),msz,var);
  }
  return q;
}


template<typename CNTNR> string subst(transaction_base &t,
	string q,
	const CNTNR &patterns)
{
  return subst(t, q, patterns.begin(), patterns.end());
}


void test_prepared_statement(transaction_base &T)
{
  connection_base &C(T.conn());

  const string
	Q_readpgtables = "SELECT * FROM pg_tables",
	Q_seetable = Q_readpgtables + " WHERE tablename = $1",
	Q_seetables = Q_seetable + " OR tablename = $2";

  try
  {
    PQXX_CHECK(
	!(T.prepared("CountToTen").exists()),
	"Nonexistent prepared statement thinks it exists.");

    // Prepare a simple statement.
    C.prepare("CountToTen", "SELECT * FROM generate_series(1, 10)");

    PQXX_CHECK(
	T.prepared("CountToTen").exists(),
	"Prepared statement thinks it doesn't exist.");

    // See if a basic prepared statement works just like a regular query.
    PQXX_CHECK_EQUAL(
	T.prepared("CountToTen").exec(),
	T.exec("SELECT * FROM generate_series(1, 10)"),
	"CountToTen");
  }
  catch (const feature_not_supported &)
  {
    cout << "Backend version does not support prepared statements.  Skipping."
         << endl;
    return;
  }

  // Try prepare_now() on an already prepared statement.
  C.prepare_now("CountToTen");

  // Drop prepared statement.
  C.unprepare("CountToTen");

  PQXX_CHECK_THROWS(
	C.prepare_now("CountToTen"),
	exception,
	"prepare_now() succeeded on dropped statement.");

  // It's okay to unprepare a statement repeatedly.
  C.unprepare("CountToTen");
  C.unprepare("CountToTen");

  // Executing an unprepared statement fails.
  PQXX_CHECK_THROWS(
	T.prepared("CountToTen").exec(),
	exception,
	"Execute unprepared statement didn't fail.");

  // Once unprepared, a statement can be prepared and used again.
  C.prepare("CountToTen", "SELECT generate_series FROM generate_series(1, 10)");
  C.prepare_now("CountToTen");
  COMPARE_RESULTS("CountToTen_2",
	T.prepared("CountToTen").exec(),
	T.exec("SELECT * FROM generate_series(1, 10)"));

  // Double preparation of identical statement should be ignored...
  C.prepare("CountToTen", "SELECT generate_series FROM generate_series(1, 10)");
  COMPARE_RESULTS("CountToTen_double",
	T.prepared("CountToTen").exec(),
	T.exec("SELECT * FROM generate_series(1, 10)"));

  // ...But a modified definition shouldn't.
  PQXX_CHECK_THROWS(
	C.prepare(
		"CountToTen",
		"SELECT generate_series FROM generate_series(1, 11)"),
	exception,
	"Bad redefinition of statement went unnoticed.");

  // Test prepared statement with parameter.

  C.prepare("CountUpToTen", "SELECT * FROM generate_series($1, 10)");

  vector<int> args;
  args.push_back(2);
  COMPARE_RESULTS("CountUpToTen_seq",
	T.prepared("CountUpToTen")(args[0]).exec(),
	T.exec(subst(T, "SELECT * FROM generate_series($1, 10)", args)));

  // Test prepared statement with 2 parameters.

  C.prepare("CountRange", "SELECT * FROM generate_series($1::int, $2::int)");
  COMPARE_RESULTS("CountRange_seq",
      T.prepared("CountRange")(2)(5).exec(),
      T.exec("SELECT * FROM generate_series(2, 5)"));

  // Test prepared statement with a null parameter.
  vector<const char *> ptrs;
  ptrs.push_back(0);
  ptrs.push_back("99");

  COMPARE_RESULTS("CountRange_null1",
	T.prepared("CountRange")(ptrs[0])(ptrs[1]).exec(),
	T.exec("SELECT * FROM generate_series(NULL, 99)"));

  // Test prepared statement with a binary parameter.
  C.prepare("GimmeBinary", "SELECT $1::bytea");

  const binarystring bin_data(string("x \x01 \x02 \xff y"));

  PQXX_CHECK_EQUAL(
	binarystring(T.prepared("GimmeBinary")(bin_data).exec()[0][0]).str(),
	bin_data.str(),
	"Binary parameter was mangled somewhere along the way.");

  const binarystring nully("x\0y", 3);
  PQXX_CHECK_EQUAL(
	binarystring(T.prepared("GimmeBinary")(nully).exec()[0][0]).str(),
	nully.str(),
	"Binary string breaks on nul byte.");

  // Test unnamed prepared statement.
  C.prepare("SELECT 2*$1");
  int outcome = T.prepared()(9).exec()[0][0].as<int>();
  PQXX_CHECK_EQUAL(outcome, 18, "Unnamed prepared statement went mad.");

  // Redefine unnamed prepared statement.  Does not need to be unprepared
  // first.
  C.prepare("SELECT 2*$1 + $2");
  outcome = T.prepared()(9)(2).exec()[0][0].as<int>();
  PQXX_CHECK_EQUAL(outcome, 20, "Unnamed statement not properly redefined.");

  // Unlike how the unnamed prepared statement works in libpq, we can issue
  // other queries and then re-use the unnamed prepared statement.
  T.exec("SELECT 12");
  outcome = T.prepared()(3)(1).exec()[0][0].as<int>();
  PQXX_CHECK_EQUAL(outcome, 7, "Unnamed statement isn't what it was.");
}
} // namespace

PQXX_REGISTER_TEST(test_prepared_statement)
