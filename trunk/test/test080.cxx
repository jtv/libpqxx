#include <iostream>
#include <stdexcept>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablereader>
#include <pqxx/nontransaction>

using namespace PGSTD;
using namespace pqxx;

// TODO: Permit failure on pre-7.3 backends

// Test program for libpqxx.  Read pqxxevents table using a tablereader, with a
// column list specification.  This requires a backend version of at least 7.3.
//
// Usage: test080 [connect-string]
//
// Where connect-string is a set of connection options in PostgreSQL's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend
// running on host foo.bar.net, logging in as user smith.
//
// The default table name is "pqxxevents" as used by other test programs.
// PostgreSQL currently implements pg_tables as a view, which cannot be read by
// using the COPY command.  Otherwise, pg_tables would have made a better
// default value here.
int main(int, char *argv[])
{
  try
  {
    const string Table = "pqxxevents";

    items<> Columns("year","event");
    items<> RevColumns;
    RevColumns("event")("year");

    connection C(argv[1]);
    nontransaction T(C);

    vector<string> R, First;

    tablereader Stream(T, Table, Columns.begin(), Columns.end());

    // Read results into string vectors and print them
    for (int n=0; (Stream >> R); ++n)
    {
      // Keep the first row for later consistency check
      if (n == 0) First = R;

      cout << n << ":\t";
      for (vector<string>::const_iterator i = R.begin(); i != R.end(); ++i)
        cout << *i << '\t';
      cout << endl;
      R.clear();
    }
    Stream.complete();

    // Verify the contents we got for the first row
    if (!First.empty())
    {
      tablereader Verify(T, Table, RevColumns.begin(), RevColumns.end());
      string Line;

      if (!Verify.get_raw_line(Line))
	throw logic_error("tablereader got rows the first time around, "
	                  "but none the second time!");

      cout << "First tuple was: " << endl << Line << endl;

      Verify.tokenize(Line, R);
      const vector<string> &Rconst(R);
      vector<string> S;
      for (vector<string>::const_reverse_iterator i = Rconst.rbegin();
	   i != Rconst.rend();
	   ++i)
	S.push_back(*i);
      if (S != First)
        throw logic_error("Got different results re-parsing first tuple!");
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

