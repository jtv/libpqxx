#include <pqxx/compiler.h>

#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;

namespace
{

string GetDatestyle(connection_base &C)
{
  return nontransaction(C, "getdatestyle").get_variable("DATESTYLE");
}

string SetDatestyle(connection_base &C, string style)
{
  C.set_variable("DATESTYLE", style);
  const string fullname = GetDatestyle(C);
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

void CheckDatestyle(connection_base &C, string expected)
{
  CompareDatestyles(GetDatestyle(C), expected);
}

void RedoDatestyle(connection_base &C, string style, string expected)
{
  CompareDatestyles(SetDatestyle(C, style), expected);
}

void ActivationTest(connection_base &C, string style, string expected)
{
  RedoDatestyle(C, style, expected);
  cout << "Deactivating connection..." << endl;
  C.deactivate();
  CheckDatestyle(C, expected);
  cout << "Reactivating connection..." << endl;
  C.activate();
  CheckDatestyle(C, expected);
}

}

// Example program for libpqxx.  Test session variables with asyncconnection.
//
// Usage: test064 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    asyncconnection C(argv[1]);

    if (GetDatestyle(C).empty())
      throw logic_error("Initial datestyle not set");

    const string ISOname = SetDatestyle(C, "ISO");
    const string SQLname = SetDatestyle(C, "SQL");

    if (ISOname == SQLname)
      throw logic_error("Datestyles SQL and ISO both show as '"+ISOname+"'");

    RedoDatestyle(C, "SQL", SQLname);

    ActivationTest(C, "ISO", ISOname);
    ActivationTest(C, "SQL", SQLname);

     // Prove that setting an unknown variable causes an error, as expected
    try
    {
      C.set_variable("NONEXISTANT_VARIABLE_I_HOPE", "1");
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

