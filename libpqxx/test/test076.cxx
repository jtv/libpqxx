#include <pqxx/compiler.h>

#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Test string conversion routines.
//
// Usage: test076 [connect-string]
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
    nontransaction T(C, "test76");
    result RFalse = T.exec("SELECT 1=0"),
    	   RTrue  = T.exec("SELECT 1=1");
    bool False, True;
    from_string(RFalse[0][0], False);
    from_string(RTrue[0][0],  True);
    if (False) throw runtime_error("False bool converted to true");
    if (!True) throw runtime_error("True bool converted to false");

    RFalse = T.exec("SELECT " + to_string(False));
    RTrue  = T.exec("SELECT " + to_string(True));
    from_string(RFalse[0][0], False);
    from_string(RTrue[0][0],  True);
    if (False) throw runtime_error("False constant converted to true");
    if (!True) throw runtime_error("True constant converted to false");

    const short svals[] = { -1, 1, 999, -32767, -32768, 32767, 0 };
    for (int i=0; svals[i]; ++i)
    {
      short s;
      from_string(to_string(svals[i]), s);
      if (s != svals[i])
	throw runtime_error("Short/string conversion not bijective");
      from_string(T.exec("SELECT " + to_string(svals[i]))[0][0].c_str(), s);
      if (s != svals[i])
	throw runtime_error("Feeding " + to_string(svals[i]) + " "
	    "through the backend yielded " + to_string(s));
    }

    const unsigned short uvals[] = { 1, 999, 32767, 32768, 65535, 0 };
    for (int i=0; uvals[i]; ++i)
    {
      unsigned short u;
      from_string(to_string(uvals[i]), u);
      if (u != uvals[i])
	throw runtime_error("Unsigned short/string conversion not bijective");
      from_string(T.exec("SELECT " + to_string(uvals[i]))[0][0].c_str(), u);
      if (u != uvals[i])
	throw runtime_error("Feeding unsigned " + to_string(uvals[i]) + " "
	    "through the backend yielded " + to_string(u));
    }
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

