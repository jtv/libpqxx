#include <pqxx/compiler.h>

#include <iostream>

#include <pqxx/binarystring>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;

// Example program for libpqxx.  Test binarystring functionality.
//
// Usage: test062 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    const string TestStr = "Nasty\n\030Test\n\t String\r\0 With Trailer";

    connection C(argv[1]);
    work T(C, "test62");

    T.exec("CREATE TEMP TABLE pqxxbin (binfield bytea)");
    const string Esc = escape_binary(TestStr);
    T.exec("INSERT INTO pqxxbin VALUES ('" + Esc + "')");

    result R = T.exec("SELECT * from pqxxbin");
    binarystring B( R.at(0).at(0) );

    if (B.empty())
      throw logic_error("Binary string became empty in conversion");

    if (B.size() != TestStr.size())
      throw logic_error("Binary string got changed from " + 
	  ToString(TestStr.size()) + " to " + ToString(B.size()) + " bytes");

    if (strncmp(TestStr.c_str(), B.c_ptr(), B.size()) != 0)
      throw logic_error("Binary string was changed before first zero byte: "
	  "'" + string(B.c_ptr(), B.size()) + "'");

    binarystring::const_iterator c;
    binarystring::size_type i;
    for (i=0, c=B.begin(); i<B.size(); ++i, ++c)
    {
      if (c == B.end())
	throw logic_error("Premature end to binary string at " + ToString(i));

      if (B.data()[i] != TestStr.at(i))
	throw logic_error("Binary string byte " + ToString(i) + " "
	    "got changed from '" + ToString(TestStr[i]) + "' "
	    "to '" + ToString(B.data()[i]) + "'");
      if (B.at(i) != B.data()[i])
	throw logic_error("Inconsistent byte at offset " + ToString(i) + ": "
	    "operator[] says '" + ToString(B.at(i)) + "', "
	    "data() says '" + ToString(B.data()[i]) + "'");
    }
    if (c != B.end())
      throw logic_error("end() of binary string not reached");

    binarystring::const_reverse_iterator r;
    for (i=B.length(), r=B.rbegin(); i>0; --i, ++r)
    {
      if (r == B.rend())
	throw logic_error("Premature rend to binary string at " + ToString(i));

      if (B[i-1] != TestStr.at(i-1))
	throw logic_error("Reverse iterator differs at " + ToString(i));
    }
    if (r != B.rend())
      throw logic_error("rend() of binary string not reached");

    if (B.str() != TestStr)
      throw logic_error("Binary string got mangled: '" + B.str() + "'");
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

