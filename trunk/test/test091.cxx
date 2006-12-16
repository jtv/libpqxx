#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/cursor>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// "Adopted absolute cursor" test program for libpqxx.  Create SQL cursor, wrap
// it in a cursor, then use it to fetch data and check for consistent results.
//
// Usage: test091 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    const string Table = "pg_tables", Key = "tablename";

    connection c(argv[1]);
    transaction<serializable> t(c, "test91");

    absolute_cursor<cursor_base::random_access, cursor_base::read_only>
      a(&t, "SELECT * FROM pg_tables", "t91");

    if (a.pos() != 0)
      throw logic_error("Fresh cursor is at position " + to_string(a.pos()));

    result all = a.fetch(cursor_base::all());
    const cursor_base::difference_type backwards =
	-cursor_base::difference_type(all.size());

    if (!c.server_version())
    {
      cout << "Can't see whether backend supports scrolling cursors.  Skipping."
           << endl;
      return 0;
    }

    cursor_base::difference_type offset;

    const cursor_base::difference_type backabs = a.move_to(0);
    if (result::size_type(backabs) != all.size())
    {
      if (c.server_version() < 70300)
      {
        // Prior to that, cursors became inert after fetching all data
        cerr << "Backend version limits cursor mobility, not testing." << endl;
        return 0;
      }
      throw logic_error("Moved " + to_string(backabs) + " rows back, "
	"but expected " + to_string(all.size()));
    }

    const cursor_base::difference_type fwd = a.move_to(cursor_base::all());
    if (backabs != fwd)
      throw logic_error("Moved " + to_string(backabs) + " rows back "
	"but " + to_string(fwd) + " rows forward");

    cursor_base::difference_type backabs2 = a.move_to(0, offset);

    if (backabs2 != backabs)
      throw logic_error("Inconsistent backwards movements: " +
	to_string(backabs) + " rows vs. " + to_string(backabs2));
    if (result::size_type(backabs) != all.size())
      throw logic_error("Got " + to_string(all.size()) + " rows forwards, "
	"but " + to_string(backabs) + " moving backwards");
    if (offset != backwards-1)
      throw logic_error("Expected to move " + to_string(backwards-1) + " "
	"rows, but moved " + to_string(offset));
    if (backabs != cursor_base::difference_type(all.size()))
      throw logic_error("Backwards move reported " + to_string(backabs) + " "
	"rows instead of " + to_string(all.size()));

    result allagain = a.fetch(cursor_base::all(), offset);
    if (allagain.size() != all.size())
      throw logic_error("Inconsistent result from cursor: " +
	to_string(all.size()) + " rows vs. " + to_string(allagain.size()));
    if (result::size_type(offset) != all.size()+1)
      throw logic_error("Unexpected offset: " + to_string(offset) + " "
	"(expected " + to_string(all.size()+1) + ")");

    offset = a.move_to(1);
    if (offset != cursor_base::difference_type(all.size()))
      throw logic_error("Unexpected displacement moving to position 1: "
	"expected " + to_string(1-long(all.size())) + ", "
	"got " + to_string(offset));

    result r = a.fetch(1);
    if (r[0] != all[1])
      throw logic_error("Unexpected data at position 1");
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

