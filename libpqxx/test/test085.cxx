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

template<typename STR1, typename STR2>
void cmp_exec(transaction_base &T, string desc, STR1 name, STR2 def)
{
  compare_results(desc, T.exec_prepared(name), T.exec(def));
}


// Substitute variables in raw query.  This is not likely to be very robust,
// but it should do for just this test.  The main shortcomings are escaping,
// and not knowing when to quote the variables.
// Note we need to do the replacement backwards (meaning forward_only
// iterators won't do!) to avoid substituting "$12" as "$1" first.
template<typename ITER> string subst(string q, ITER patbegin, ITER patend)
{
  int i = distance(patbegin, patend);
  for (ITER arg = --patend; i > 0; --arg, --i)
  {
    const string marker = "$" + to_string(i),
	  var = "'" + to_string(*arg) + "'";
    const string::size_type msz = marker.size();
    while (q.find(marker) != string::npos) q.replace(q.find(marker),msz,var);
  }
  return q;
}


template<typename CNTNR> string subst(string q, const CNTNR &patterns)
{
  return subst(q, patterns.begin(), patterns.end());
}


template<typename STR1, typename STR2, typename ITER>
void cmp_exec(transaction_base &T,
    string desc,
    STR1 name,
    STR2 def,
    ITER argsbegin,
    ITER argsend)
{
  compare_results(desc, 
      T.exec_prepared(name, argsbegin, argsend), 
      T.exec(subst(def, argsbegin, argsend)));
}


template<typename STR1, typename STR2, typename CNTNR>
void cmp_exec(transaction_base &T,
    string desc,
    STR1 name,
    STR2 def,
    CNTNR args)
{
  compare_results(desc, T.exec_prepared(name,args), T.exec(subst(def,args)));
}


} // namespace


// Test program for libpqxx.  Define and use prepared statements.
//
// Usage: test085
int main()
{
  try
  {
    const string QN_readpgtables = "readpgtables",
	  Q_readpgtables = "SELECT * FROM pg_tables",
	  QN_seetable = "seetable",
	  Q_seetable = Q_readpgtables + " WHERE tablename=$1",
	  QN_seetables = "seetables",
	  Q_seetables = Q_seetable + " OR tablename=$2";

    lazyconnection C;
    cout << "Preparing a simple statement..." << endl;
    C.prepare(QN_readpgtables, Q_readpgtables);
    nontransaction T(C, "test85");

    // See if a basic prepared statement runs consistently with a regular query
    cout << "Basic correctness check on prepared statement..." << endl;
    cmp_exec(T,QN_readpgtables,QN_readpgtables,Q_readpgtables);

    // Pro forma check: same thing but with name passed as C-style string
    cmp_exec(T,QN_readpgtables+"_char",QN_readpgtables.c_str(),Q_readpgtables);

    cout << "Dropping prepared statement..." << endl;
    C.unprepare(QN_readpgtables);

    // Just to try and confuse things, "unprepare" twice
    cout << "Testing error detection and handling..." << endl;
    try { C.unprepare(QN_readpgtables); }
    catch (const exception &e) { cout << "(Expected) " << e.what() << endl; }

    // Verify that attempt to execute unprepared statement fails
    bool failsOK = true;
    try { T.exec_prepared(QN_readpgtables); failsOK = false; }
    catch (const exception &e) { cout << "(Exepected) " << e.what() << endl; }
    if (!failsOK) throw logic_error("Execute unprepared statement didn't fail");

    // Re-prepare the same statement and test again
    C.prepare(QN_readpgtables, Q_readpgtables);
    cmp_exec(T,QN_readpgtables+"_2", QN_readpgtables, Q_readpgtables);

    // Double preparation of identical statement should be ignored...
    C.prepare(QN_readpgtables, Q_readpgtables);
    cmp_exec(T,QN_readpgtables+"_double", QN_readpgtables, Q_readpgtables);

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


    cout << "Testing parameterized prepared-statement functions..." << endl;
    // Try definitions of same statement with empty parameter lists
    const list<string> dummy;
    C.unprepare(QN_readpgtables);
    C.prepare(QN_readpgtables, Q_readpgtables, dummy.begin(), dummy.end());
    cmp_exec(T,QN_readpgtables+"_seq", QN_readpgtables, Q_readpgtables);
    C.unprepare(QN_readpgtables);
    C.prepare(QN_readpgtables, Q_readpgtables, list<string>());
    cmp_exec(T,QN_readpgtables+"_cntnr", QN_readpgtables, Q_readpgtables);

    // Try executing with empty argument lists
    cmp_exec(T,
	QN_readpgtables+" with empty argument sequence",
	QN_readpgtables,
	Q_readpgtables,
	dummy.begin(),
	dummy.end());
    cmp_exec(T,
	QN_readpgtables+" with empty argument container",
	QN_readpgtables,
	Q_readpgtables,
	dummy);
    cmp_exec(T,
	QN_readpgtables+" with empty argument container and char name",
	QN_readpgtables.c_str(),
	Q_readpgtables,
	dummy);


    cout << "Testing prepared statement with parameter..." << endl;

    list<string> parms, args;
    parms.push_back("varchar");
    C.prepare(QN_seetable, Q_seetable, parms);
    args.push_back("pg_type");
    cmp_exec(T,
	QN_seetable+"_seq",
	QN_seetable,
	Q_seetable,
	args.begin(),
	args.end());
    cmp_exec(T, QN_seetable+"_cntnr", QN_seetable, Q_seetable, args);

    cout << "Testing prepared statement with 2 parameters..." << endl;

    parms.push_back("varchar");
    C.prepare(QN_seetables, Q_seetables, parms.begin(), parms.end());
    args.push_back("pg_index");
    cmp_exec(T,
	QN_seetables+"_seq",
	QN_seetables,
	Q_seetables,
	args.begin(),
	args.end());
    cmp_exec(T, QN_seetables+"_cntnr", QN_seetables, Q_seetables, args);

    cout << "Done." << endl;
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


