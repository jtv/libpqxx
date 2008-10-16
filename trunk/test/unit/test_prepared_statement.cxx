#include <iostream>
#include <list>

#include <pqxx/compiler-internal.hxx>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Define and use prepared statements.

#define COMPARE_RESULTS(name, lhs, rhs) \
  PQXX_CHECK_EQUAL(	\
	rhs,		\
	lhs, 		\
	"Executing " name " as prepared statement yields different results."); \
  PQXX_CHECK_EQUAL(lhs.empty(), false, "Result for " name " is empty.")

namespace
{
string stringize(transaction_base &t, const string &arg)
{
  return "'" + t.esc(arg) + "'";
}


string stringize(transaction_base &t, const char arg[])
{
  return arg ? stringize(t,string(arg)) : "null";
}


string stringize(transaction_base &t, char arg[])
{
  return arg ? stringize(t, string(arg)) : "null";
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


void test_prepared_statement(connection_base &C, transaction_base &T)
{
  /* A bit of nastiness in prepared statements: on 7.3.x backends we can't
   * compare pg_tables.tablename to a string.  We work around this by using
   * the LIKE operator.
   *
   * Later backend versions do not suffer from this problem.
   */
  const string
	Q_readpgtables = "SELECT * FROM pg_tables",
	Q_seetable = Q_readpgtables + " WHERE tablename LIKE $1",
	Q_seetables = Q_seetable + " OR tablename LIKE $2";

  try
  {
    // Prepare a simple statement
    C.prepare("ReadPGTables", Q_readpgtables);

    // See if a basic prepared statement works just like a regular query
    PQXX_CHECK_EQUAL(
	T.prepared(string("ReadPGTables")).exec(),
	T.exec(Q_readpgtables),
	"ReadPGTables");
  }
  catch (const feature_not_supported &e)
  {
    cout << "Backend version does not support prepared statements.  Skipping."
         << endl;
    return;
  }

  // Try prepare_now() on an already prepared statement
  C.prepare_now("ReadPGTables");

  // Pro forma check: same thing but with name passed as C-style string
  COMPARE_RESULTS("ReadPGTables_char",
	T.prepared("ReadPGTables").exec(),
	T.exec(Q_readpgtables));

  cout << "Dropping prepared statement..." << endl;
  C.unprepare("ReadPGTables");

  PQXX_CHECK_THROWS(
	C.prepare_now("ReadPGTables"),
	exception,
	"prepare_now() succeeded on dropped statement.");

  // Just to try and confuse things, "unprepare" twice
  cout << "Testing error detection and handling..." << endl;
  try { C.unprepare("ReadPGTables"); }
  catch (const exception &e) { cout << "(Expected) " << e.what() << endl; }

  // Verify that attempt to execute unprepared statement fails
  PQXX_CHECK_THROWS(
	T.prepared("ReadPGTables").exec(),
	exception,
	"Execute unprepared statement didn't fail.");

  // Re-prepare the same statement and test again
  C.prepare("ReadPGTables", Q_readpgtables);
  C.prepare_now("ReadPGTables");
  COMPARE_RESULTS("ReadPGTables_2",
	T.prepared("ReadPGTables").exec(),
	T.exec(Q_readpgtables));

  // Double preparation of identical statement should be ignored...
  C.prepare("ReadPGTables", Q_readpgtables);
  COMPARE_RESULTS("ReadPGTables_double",
	T.prepared("ReadPGTables").exec(),
	T.exec(Q_readpgtables));

  // ...But a modified definition shouldn't
  PQXX_CHECK_THROWS(
	C.prepare("ReadPGTables", Q_readpgtables + " ORDER BY tablename"),
	exception,
	"Bad redefinition of statement went unnoticed.");

  cout << "Testing prepared statement with parameter..." << endl;

  C.prepare("SeeTable", Q_seetable)("varchar", pqxx::prepare::treat_string);

  vector<string> args;
  args.push_back("pg_type");
  COMPARE_RESULTS("SeeTable_seq",
	T.prepared("SeeTable")(args[0]).exec(),
	T.exec(subst(T,Q_seetable,args)));

  cout << "Testing prepared statement with 2 parameters..." << endl;

  C.prepare("SeeTables", Q_seetables)
      ("varchar",pqxx::prepare::treat_string)
      ("varchar",pqxx::prepare::treat_string);
  args.push_back("pg_index");
  COMPARE_RESULTS("SeeTables_seq",
      T.prepared("SeeTables")(args[0])(args[1]).exec(),
      T.exec(subst(T,Q_seetables,args)));

  cout << "Testing prepared statement with a null parameter..." << endl;
  vector<const char *> ptrs;
  ptrs.push_back(0);
  ptrs.push_back("pg_index");
  COMPARE_RESULTS("SeeTables_null1",
	T.prepared("SeeTables")(ptrs[0])(ptrs[1]).exec(),
	T.exec(subst(T,Q_seetables,ptrs)));
  COMPARE_RESULTS("SeeTables_null2",
	T.prepared("SeeTables")(ptrs[0])(ptrs[1]).exec(),
	T.prepared("SeeTables")()(ptrs[1]).exec());
  COMPARE_RESULTS("SeeTables_null3",
	T.prepared("SeeTables")(ptrs[0])(ptrs[1]).exec(),
	T.prepared("SeeTables")("somestring",false)(ptrs[1]).exec());
  COMPARE_RESULTS("SeeTables_null4",
	T.prepared("SeeTables")(ptrs[0])(ptrs[1]).exec(),
	T.prepared("SeeTables")(42,false)(ptrs[1]).exec());
  COMPARE_RESULTS("SeeTables_null5",
	T.prepared("SeeTables")(ptrs[0])(ptrs[1]).exec(),
	T.prepared("SeeTables")(0,false)(ptrs[1]).exec());

  cout << "Testing wrong numbers of parameters..." << endl;
  PQXX_CHECK_THROWS(
	T.prepared("SeeTables")()()("hi mom!").exec(),
	exception,
	"No error for too many parameters.");

  PQXX_CHECK_THROWS(
	T.prepared("SeeTables")("who, me?").exec(),
	exception,
	"No error for too few parameters.");
}
} // namespace

PQXX_REGISTER_TEST(test_prepared_statement)
