#include <cassert>
#include <cstring>
#include <iostream>
#include <list>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test binary parameters to prepared statements.
//
// Usage: test092
int main()
{
  try
  {
    const char databuf[] = "Test\0data";
    const string data(databuf, sizeof(databuf));
    assert(data.size() > strlen(databuf));

    const string Table = "pqxxbin", Field = "binfield", Stat = "nully";

    lazyconnection C;
    work T(C, "test92");
    T.exec("CREATE TEMP TABLE " + Table + " (" + Field + " BYTEA)");

    if (!C.supports(connection_base::cap_prepared_statements))
    {
      cout << "Backend version does not support prepared statements.  Skipping."
           << endl;
      return 0;
    }

    C.prepare(Stat, "INSERT INTO " + Table + " VALUES ($1)")
	("BYTEA", pqxx::prepare::treat_binary);
    T.prepared(Stat)(data).exec();

    const result L( T.exec("SELECT length("+Field+") FROM " + Table) );
    if (L[0][0].as<size_t>() != data.size())
      throw logic_error("Inserted " + to_string(data.size()) + " bytes, "
	"but " + L[0][0].as<string>() + " arrived");

    const result R( T.exec("SELECT " + Field + " FROM " + Table) );

    const binarystring roundtrip(R[0][0]);

    if (string(roundtrip.str()) != data)
      throw logic_error("Sent " + to_string(data.size()) + " bytes "
	"of binary data, got " + to_string(roundtrip.size()) + " back: "
	"'" + roundtrip.str() + "'");

    if (roundtrip.size() != data.size())
      throw logic_error("Binary string reports wrong size: " +
	to_string(roundtrip.size()) + " "
	"(expected " + to_string(data.size()) + ")");

    // People seem to like the multi-line invocation style, where you get your
    // invocation object first, then add parameters in separate C++ statements.
    // As John Mudd found, that used to break the code.  Let's test it.
    T.exec("CREATE TEMP TABLE tuple (one INTEGER, two VARCHAR)");

    pqxx::prepare::declaration d(
	C.prepare("maketuple", "INSERT INTO tuple VALUES ($1, $2)") );
    d("INTEGER", pqxx::prepare::treat_direct);
    d("VARCHAR", pqxx::prepare::treat_string);

    pqxx::prepare::invocation i( T.prepared("maketuple") );
    const string f = "frobnalicious";
    i(6);
    i(f);
    i.exec();

    const result t( T.exec("SELECT * FROM tuple") );
    if (t.size() != 1)
      throw logic_error("Expected 1 tuple, got " + to_string(t.size()));
    if (t[0][0].as<string>() != "6")
      throw logic_error("Expected value 6, got " + t[0][0].as<string>());
    if (t[0][1].c_str() != f)
      throw logic_error("Expected string '" + f + "', "
	"got '" + t[0][1].c_str() + "'");
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


