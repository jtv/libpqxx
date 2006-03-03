#include <iostream>
#include <vector>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Create a table of numbers, write data to it using
// a tablewriter back_insert_iterator, then verify the table's contents using 
// field iterators
//
// Usage: test083 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    const string Table = "pqxxnumbers";

    items<items<int> > contents;
    for (int x=0; x<10; ++x)
    {
      items<int> n(x+1);
      contents.push_back(n);
    }

    cout << "Dropping old " << Table << endl;
    try
    {
      nontransaction Drop(C, "drop_" + Table);
      Drop.exec("DROP TABLE " + Table);
    }
    catch (const sql_error &e)
    {
      cout << "(Expected) Couldn't drop table: " << e.what() << endl
	   << "Query was: " << e.query() << endl;
    }

    work T(C, "test83");
    T.exec("CREATE TABLE " + Table + "(num INTEGER)");

    tablewriter W(T, Table);
    back_insert_iterator<tablewriter> b(W);
    items<items<int> >::const_iterator i = contents.begin();
    *b = *i++;
    ++b;
    *b++ = *i++;
    back_insert_iterator<tablewriter> c(W);
    c = b;
    *c++ = *i;
    W.complete();

    const result R = T.exec("SELECT * FROM " + Table + " ORDER BY num DESC");
    for (result::const_iterator r = R.begin(); r != R.end(); --i, ++r)
      if (r->at(0).as(0) != (*i)[0])
	throw logic_error("Writing numbers with tablewriter went wrong: "
	    "expected " + to_string((*i)[0]) + ", found " + r[0].c_str());
  
    T.commit();
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

