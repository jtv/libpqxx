#include <pqxx/compiler.h>

#include <iostream>
#include <vector>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Compare const_reverse_iterator iteration of
// a result to a regular, const_iterator iteration.
//
// Usage: test075 [connect-string]
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
    work W(C, "test75");
    const result R( W.exec("SELECT year FROM pqxxevents") );

    vector<string> contents;
    for (result::const_iterator i=R.begin(); i!=R.end(); ++i)
      contents.push_back(i->at(0).as<string>());
    cout << ToString(contents.size()) << " years read" << endl;

    if (contents.size() != vector<string>::size_type(R.size()))
      throw logic_error("Got " + ToString(contents.size()) + " values "
	  "out of result with size " + ToString(R.size()));

    for (result::size_type i=0; i<R.size(); ++i)
      if (contents.at(i) != R.at(i).at(0).c_str())
	throw logic_error("Inconsistent iteration: '" + contents[i] + "' "
	    "became '" + R[i][0].as<string>());
    cout << ToString(R.size()) << " years checked" << endl;

#ifdef PQXX_HAVE_REVERSE_ITERATOR
    // Now verify that reverse iterator also sees the same results...
    vector<string>::reverse_iterator l = contents.rbegin();
    for (result::const_reverse_iterator i=R.rbegin(); i!=R.rend(); ++i)
    {
      if (*l != i->at(0).c_str())
	throw logic_error("Inconsistent reverse iteration: "
	    "'" + *l + "' became '" + (*i)[0].as<string>() + "'");
      ++l;
    }

    if (l != contents.rend())
      throw logic_error("Reverse iteration of result ended too soon");
#endif	// PQXX_HAVE_REVERSE_ITERATOR

    if (R.empty())
      throw runtime_error("No years found in events table, can't test!");
  }
  catch (const sql_error &e)
  {
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

