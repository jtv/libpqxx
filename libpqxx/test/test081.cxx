#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/cursor>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Read table pqxxevents through a cursor.
//
// Usage: test081 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{
// Compare contents of "part" with those of "org," starting at offset "here"
void cmp_results(const result &org, result::size_type &here, const result &part)
{
  if (here + part.size() > org.size())
    throw logic_error("Cursor returned more than expected " + 
	to_string(org.size()) + " rows");
  if (part[0].size() != org[0].size())
    throw logic_error("Expected " + to_string(org[0].size()) + " columns, "
	"got " + to_string(part[0].size()));

  for (result::size_type row = 0; row < part.size(); ++row, ++here)
  {
    for (result::tuple::size_type field = 0; field < part[row].size(); ++field)
      if (string(part[row][field].c_str()) != string(org[here][field].c_str()))
        throw logic_error("Row " + to_string(here) + ", "
		"field " + to_string(field) + ": "
		"expected '" + org[here][field].c_str() + "', "
		"got '" + part[row][field].c_str() + "'");
  }
}

bool get(icursorstream &C, result &R, result::size_type expectedrows)
{
  if (!(C >> R))
  {
    if (!R.empty()) throw logic_error("Finished cursor returned " +
	to_string(R.size()) + " rows");
    return false;
  }

  if (R.empty()) throw logic_error("Unfinished cursor returned empty result");
  if (R.size() > expectedrows)
    throw logic_error("Expected at most " + to_string(expectedrows) + " "
	"rows, got " + to_string(R.size()));

  return true;
}


void start(icursorstream &C, result::size_type &here)
{
  here = 0;
  cout << "Testing cursor " << C.name() << endl;
}

void finish(icursorstream &C,
    result::size_type expectedrows,
    result::size_type here)
{
  result R;
  get(C, R, 0);
  if (C) throw logic_error("Cursor in inconsistent EOF state");
  if (!!C) throw logic_error("Broken operator ! on cursor");
  if (here < expectedrows)
    throw logic_error("Expected " + to_string(expectedrows) + " rows, "
	"got " + to_string(here));
}

} // namespace


int main(int, char *argv[])
{
  try
  {
    const string Table = "pqxxevents";

    connection C(argv[1]);

    work T(C, "test81");

    const string Query("SELECT * FROM " + Table + " ORDER BY year");

    const result Ref(T.exec(Query));
    if (Ref.empty())
      throw runtime_error("Table " + Table + " appears to be empty.  "
	  "Cannot test with an empty table, sorry.");


    result R;
    result::size_type here;

    // Simple test: read back results 1 row at a time
    icursorstream Cur1(T, Query, "singlestep", cursor_base::next());

    if (Cur1.stride() != cursor_base::next())
      throw logic_error("Expected stride to be " + 
	  to_string(cursor_base::next()) + ", found " +
	  to_string(Cur1.stride()));

    int rows=0;
    for (start(Cur1, here); get(Cur1, R, 1); ++rows) cmp_results(Ref, here, R);
    finish(Cur1, Ref.size(), here);

    // Read whole table at once
    icursorstream Cur2(T, Query, "bigstep");
    Cur2.set_stride(cursor_base::all());

    if (Cur2.stride() != cursor_base::all())
      throw logic_error("Expected stride to be " + 
	  to_string(cursor_base::all()) + ", found " +
	  to_string(Cur2.stride()));

    start(Cur2, here);
    if (!get(Cur2, R, Ref.size())) throw logic_error("No data!");
    cmp_results(Ref, here, R);
    finish(Cur2, Ref.size(), here);

    // Read with varying strides
    icursorstream Cur3(T, Query, "irregular", 1);
    start(Cur3, here);
    for (result::size_type stride=1; get(Cur3, R, stride); ++stride)
    {
      cmp_results(Ref, here, R);
      Cur3.set_stride(stride+1);
    }
    finish(Cur3, Ref.size(), here);

    // Try skipping a few lines
    icursorstream Cur4(T, Query, "skippy", 2);
    start(Cur4, here);
    while (get(Cur4, R, 2))
    {
      cmp_results(Ref, here, R);
      Cur4.ignore(3);
      here += 3;
    }
    finish(Cur4, Ref.size(), here);

    // Check if we get the expected number of rows again
    icursorstream Cur5(T, Query, "count");
    if (!Cur5.ignore(rows-1))
      throw runtime_error("Could not skip " + to_string(rows-1) + " rows");
    if (!Cur5.get(R))
      throw runtime_error("Expected " + to_string(rows) + " rows, got " +
	  	to_string(rows-1));
    if (R.empty())
      throw runtime_error("Unexpected empty result at last row");
    if (Cur5.get(R))
      throw runtime_error("Ending row is nonempty");
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

