#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/cursor>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Read table pqxxevents through a cursor.  Default
// blocksize is 1; use 0 to read all rows at once.
//
// Usage: test081 [connect-string] [blocksize]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int argc, char *argv[])
{
  try
  {
    const string Table = "pqxxevents";

    int BlockSize = 1;
    if (argc > 2) from_string(argv[2], BlockSize);
    if (BlockSize == 0) BlockSize = cursor_base::ALL();

    connection C(argv[1]);

    work T(C, "test81");

    const string Query("SELECT * FROM " + Table + " ORDER BY year");

    const result Ref(T.exec(Query));
    if (Ref.empty())
      throw runtime_error("Table " + Table + " appears to be empty.  "
	  "Cannot test with an empty table, sorry.");

    icursorstream Cur(T, Query, "tablecur", BlockSize);

    result R;
    result::size_type here = 0;
    while ((Cur >> R))
    {
      if (!Cur) throw logic_error("Inconsistent cursor state!");

      if (R.size() > BlockSize)
        throw logic_error("Cursor returned " + to_string(R.size()) + " rows, "
			  "when " + to_string(BlockSize) + " "
			  "was all I asked for!");

      // Compare cursor's results with Ref
      for (result::size_type row = 0; row < R.size(); ++row, ++here)
      {
	if (here >= Ref.size())
	  throw logic_error("Cursor returned more than expected " +
	      to_string(Ref.size()) + " rows");

        for (result::tuple::size_type field = 0; field < R[row].size(); ++field)
	  if (string(R[row][field].c_str()) != string(Ref[here][field].c_str()))
	    throw logic_error("Row " + to_string(row) + ", "
		"field " + to_string(field) + ": "
		"expected '" + Ref[row][field].c_str() + "', "
		"got '" + R[row][field].c_str() + "'");
      }
    }

    if (!!Cur) throw logic_error("Inconsistent cursor state!");
    if (here < Ref.size())
      throw logic_error("Cursor returned " + to_string(here) + " rows; "
	  "expected " + to_string(Ref.size()));
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
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

