#include <cassert>
#include <iostream>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx's asyncconnection.  Asynchronously open a
// connection to the database, start a transaction, and perform a query inside 
// it.
//
// Usage: test063
int main()
{
  try
  {
    asyncconnection C;
    cout << "Connection in progress..." << endl;
    work T(C, "test63");
    result R( T.exec("SELECT * FROM pg_tables") );

    if (R.empty())
      throw logic_error("No tables found!");

    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
      cout << '\t' << to_string(c.num()) << '\t' << c[0].as(string()) << endl;

    T.commit();
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 1;
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


