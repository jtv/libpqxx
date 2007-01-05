#include <cassert>
#include <iostream>
#include <list>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void compare_results(string name, result lhs, result rhs)
{
  if (lhs != rhs)
    throw logic_error("Executing " + name + " as prepared statement "
	  "yields different results from direct execution");

  if (lhs.empty())
    throw logic_error("Results being compared are empty.  Not much point!");
}


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
  int i = distance(patbegin, patend);
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

} // namespace


// Test program for libpqxx.  Define and use prepared statements.
//
// Usage: test085
int main()
{
  try
  {
    /* A bit of nastiness in prepared statements: on 7.3.x backends we can't
     * compare pg_tables.tablename to a string.  We work around this by using
     * the LIKE operator.
     *
     * Later backend versions do not suffer from this problem.
     */
    const string QN_readpgtables = "ReadPGTables",
	  Q_readpgtables = "SELECT * FROM pg_tables",
	  QN_seetable = "SeeTable",
	  Q_seetable = Q_readpgtables + " WHERE tablename LIKE $1",
	  QN_seetables = "SeeTables",
	  Q_seetables = Q_seetable + " OR tablename LIKE $2";

    lazyconnection C;

    cout << "Preparing a simple statement..." << endl;
    C.prepare(QN_readpgtables, Q_readpgtables);
    nontransaction T(C, "test85");

    try
    {
      // See if a basic prepared statement works just like a regular query
      cout << "Basic correctness check on prepared statement..." << endl;
      compare_results(QN_readpgtables,
	T.prepared(QN_readpgtables).exec(),
	T.exec(Q_readpgtables));
    }
    catch (const exception &)
    {
      if (!C.supports(connection_base::cap_prepared_statements))
      {
        cout << "Backend version does not support prepared statements.  "
	        "Skipping."
	     << endl;
	return 0;
      }
      throw;
    }

    // Try prepare_now() on an already prepared statement
    C.prepare_now(QN_readpgtables);

    // Pro forma check: same thing but with name passed as C-style string
    compare_results(QN_readpgtables+"_char",
	T.prepared(QN_readpgtables.c_str()).exec(),
	T.exec(Q_readpgtables));

    cout << "Dropping prepared statement..." << endl;
    C.unprepare(QN_readpgtables);

    bool failed = true;
    try
    {
      disable_noticer d(C);
      C.prepare_now(QN_readpgtables);
      failed = false;
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    if (!failed)
      throw runtime_error("prepare_now() succeeded on dropped statement");


    // Just to try and confuse things, "unprepare" twice
    cout << "Testing error detection and handling..." << endl;
    try { C.unprepare(QN_readpgtables); }
    catch (const exception &e) { cout << "(Expected) " << e.what() << endl; }

    // Verify that attempt to execute unprepared statement fails
    bool failsOK = true;
    try { T.prepared(QN_readpgtables).exec(); failsOK = false; }
    catch (const exception &e) { cout << "(Expected) " << e.what() << endl; }
    if (!failsOK) throw logic_error("Execute unprepared statement didn't fail");

    // Re-prepare the same statement and test again
    C.prepare(QN_readpgtables, Q_readpgtables);
    C.prepare_now(QN_readpgtables);
    compare_results(QN_readpgtables+"_2",
	T.prepared(QN_readpgtables).exec(),
	T.exec(Q_readpgtables));

    // Double preparation of identical statement should be ignored...
    C.prepare(QN_readpgtables, Q_readpgtables);
    compare_results(QN_readpgtables+"_double",
	T.prepared(QN_readpgtables).exec(),
	T.exec(Q_readpgtables));

    // ...But a modified definition shouldn't
    try
    {
      failsOK = true;
      C.prepare(QN_readpgtables, Q_readpgtables + " ORDER BY tablename");
      failsOK = false;
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    if (!failsOK)
      throw logic_error("Bad redefinition of statement went unnoticed");

    cout << "Testing prepared statement with parameter..." << endl;

    C.prepare(QN_seetable, Q_seetable)("varchar", pqxx::prepare::treat_string);

    vector<string> args;
    args.push_back("pg_type");
    compare_results(QN_seetable+"_seq",
	T.prepared(QN_seetable)(args[0]).exec(),
	T.exec(subst(T,Q_seetable,args)));

    cout << "Testing prepared statement with 2 parameters..." << endl;

    C.prepare(QN_seetables, Q_seetables)
      ("varchar",pqxx::prepare::treat_string)
      ("varchar",pqxx::prepare::treat_string);
    args.push_back("pg_index");
    compare_results(QN_seetables+"_seq",
      T.prepared(QN_seetables)(args[0])(args[1]).exec(),
      T.exec(subst(T,Q_seetables,args)));

    cout << "Testing prepared statement with a null parameter..." << endl;
    vector<const char *> ptrs;
    ptrs.push_back(0);
    ptrs.push_back("pg_index");
    compare_results(QN_seetables+"_null1",
	T.prepared(QN_seetables)(ptrs[0])(ptrs[1]).exec(),
	T.exec(subst(T,Q_seetables,ptrs)));
    compare_results(QN_seetables+"_null2",
	T.prepared(QN_seetables)(ptrs[0])(ptrs[1]).exec(),
	T.prepared(QN_seetables)()(ptrs[1]).exec());
    compare_results(QN_seetables+"_null3",
	T.prepared(QN_seetables)(ptrs[0])(ptrs[1]).exec(),
	T.prepared(QN_seetables)("somestring",false)(ptrs[1]).exec());
    compare_results(QN_seetables+"_null4",
	T.prepared(QN_seetables)(ptrs[0])(ptrs[1]).exec(),
	T.prepared(QN_seetables)(42,false)(ptrs[1]).exec());
    compare_results(QN_seetables+"_null5",
	T.prepared(QN_seetables)(ptrs[0])(ptrs[1]).exec(),
	T.prepared(QN_seetables)(0,false)(ptrs[1]).exec());

    cout << "Testing wrong numbers of parameters..." << endl;
    try
    {
      failsOK = true;
      T.prepared(QN_seetables)()()("hi mom!").exec();
      failsOK = false;
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    if (!failsOK)
      throw logic_error("No error for too many parameters");
    try
    {
      failsOK = true;
      T.prepared(QN_seetables)("who, me?").exec();
      failsOK = false;
    }
    catch (const exception &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    if (!failsOK)
      throw logic_error("No error for too few parameters");

    cout << "Done." << endl;
  }
  catch (const feature_not_supported &e)
  {
    cout << "Backend version does not support prepared statements.  Skipping."
         << endl;
    return 0;
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 1;
  }
  catch (const exception &e)
  {
    // All exceptions thrown by libpqxx are derived from std::exception
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    // This is really unexpected (see above)
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


