#include <pqxx/compiler.h>

#include <iostream>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;

namespace
{

string GetDatestyle(transaction_base &T)
{
  return T.exec("SHOW DATESTYLE").at(0).at(0).c_str();
}

string SetDatestyle(transaction_base &T, string style)
{
  T.set_variable("DATESTYLE", style);
  const string fullname = GetDatestyle(T);
  cout << "Set datestyle to " << style << ": " << fullname << endl;
  if (fullname.empty())
    throw logic_error("Setting datestyle to " + style + " "
	"makes it an empty string");

  return fullname;
}

void CompareDatestyles(string fullname, string expected)
{
  if (fullname != expected)
    throw logic_error("Datestyle is '" + fullname + "', "
	"expected '" + expected + "'");
}

void CheckDatestyle(transaction_base &T, string expected)
{
  CompareDatestyles(GetDatestyle(T), expected);
}

void RedoDatestyle(transaction_base &T, string style, string expected)
{
  CompareDatestyles(SetDatestyle(T, style), expected);
}

}

// Example program for libpqxx.  Test local variable functionality.
//
// Usage: test061 [connect-string]
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
    work T(C, "test61");

    if (GetDatestyle(T).empty())
      throw logic_error("Initial datestyle not set");

    const string ISOname = SetDatestyle(T, "ISO");
    const string SQLname = SetDatestyle(T, "SQL");

    if (ISOname == SQLname)
      throw logic_error("Datestyles SQL and ISO both show as '"+ISOname+"'");

    RedoDatestyle(T, "SQL", SQLname);

     // Prove that setting an unknown variable causes an error, as expected
    try
    {
      T.set_variable("NONEXISTANT_VARIABLE_I_HOPE", "1");
      throw logic_error("Setting unknown variable failed to fail");
    }
    catch (const sql_error &e)
    {
      cout << "(Expected) Setting unknown variable failed" << endl;
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

