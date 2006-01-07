#include <pqxx/compiler-internal.hxx>

#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

// Don't try this at home: peeking inside libpqxx to see if we can test the
// column_table() functionality
#include <pqxx/config-internal-libpq.h>

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Perform a query and enumerate its output
// using array indexing.
//
// Usage: test002 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    // Before we really connect, test the expected behaviour of the default
    // connection type, where a failure to connect results in an immediate
    // exception rather than a silent retry.
    bool failsOK = true;
    try
    {
      connection C("totally#invalid@connect$string!?");
      failsOK = false;
    }
    catch (const broken_connection &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    if (!failsOK)
      throw logic_error("Connection failure went unnoticed!");

    // Set up connection to database
    string ConnectString = (argv[1] ? argv[1] : "");
    connection C(ConnectString);

    // Start transaction within context of connection
    work T(C, "test2");

    // Perform query within transaction
    result R( T.exec("SELECT * FROM pg_tables") );

    // Let's keep the database waiting as briefly as possible: commit now,
    // before we start processing results.  We could do this later, or since
    // we're not making any changes in the database that need to be committed,
    // we could in this case even omit it altogether.
    T.commit();

    // Since we don't need the database anymore, we can be even more 
    // considerate and close the connection now.  This is optional.
    C.disconnect();

#ifdef PQXX_HAVE_PQFTABLE
    // Ah, this version of postgres will tell you which table a column in a
    // result came from.  Let's just test that functionality...
    const oid rtable = R.column_table(0);
    if (rtable != R.column_table(result::tuple::size_type(0)))
      throw logic_error("Inconsistent result::column_table()");

    const string rcol = R.column_name(0);
    const oid crtable = R.column_table(rcol);
    if (crtable != rtable)
      throw logic_error("Field " + rcol + " comes from "
                        "'" + to_string(rtable) + "', "
	  		"but by name, result says it's from "
                        "'" + to_string(crtable) + "'");
#endif

    // Now we've got all that settled, let's process our results.
    for (result::size_type i = 0; i < R.size(); ++i)
    {
      cout << '\t' << to_string(i) << '\t' << R[i][0].c_str() << endl;

#ifdef PQXX_HAVE_PQFTABLE
      const oid ftable = R[i][0].table();
      if (ftable != rtable)
	throw logic_error("Field says it comes from "
                          "'" + to_string(ftable) + "'; "
			  "expected '" + to_string(rtable) + "'");
      const oid ttable = R[i].column_table(0);
      if (ttable != R[i].column_table(result::tuple::size_type(0)))
	throw logic_error("Inconsistent result::tuple::column_table()");
      if (ttable != rtable)
	throw logic_error("Tuple says field comes from "
                          "'" + to_string(ttable) + "'; "
	    		  "expected '" + to_string(rtable) + "'");
      const oid cttable = R[i].column_table(rcol);
      if (cttable != rtable)
	throw logic_error("Field comes from '" + to_string(rtable) + "', "
	                  "but by name, tuple says it's from '" + 
			  to_string(cttable) + "'");
#endif
    }
  }
  catch (const sql_error &e)
  {
    // If we're interested in the text of a failed query, we can write separate
    // exception handling code for this type of exception
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
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

