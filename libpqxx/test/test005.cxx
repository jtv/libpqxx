#include <iostream>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Write a predetermined data set to a table
// using a tablewriter.  This data will be used by subsequent tests.  Any data
// previously in the table will be deleted.
//
// Usage: test005 [connect-string] [tablename]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The tablename argument determines which table the data will be written to.
// If none is given, it defaults to "pqxxorgevents".
int main(int argc, char *argv[])
{
  try
  {
    // Set up a connection to the backend
    connection C(argv[1]);

    string TableName((argc > 2) ? argv[2] : "pqxxorgevents");

    // First create a separate transaction to drop old table, if any.  This may
    // fail if the table didn't previously exist.
    cout << "Dropping old " << TableName << endl;
    try
    {
      transaction<> Drop(C, "drop_" + TableName);
      Drop.exec(("DROP TABLE " + TableName).c_str());
      Drop.commit();
    }
    catch (const sql_error &e)
    {
      cerr << "(Expected) Couldn't drop table: " << e.what() << endl
	   << "Query was: " << e.query() << endl;
    }

    // Now begin new transaction to create new table & write data
    transaction<> T(C, "test5");

    T.exec(("CREATE TABLE " + 
	    TableName + 
	    "(year INTEGER, event VARCHAR)").c_str());

    // NOTE: start a nested block here to ensure that our stream W is closed 
    // before we attempt to commit our transaction T.  Otherwise we might end 
    // up committing T before all data going into W had been written.
    {
      tablewriter W(T, TableName);

      const char *const CData[][2] =
      {
        {   "71", "jtv" },
        {   "38", "time_t overflow" },
        {    "1", "'911' WTC attack" },
        {   "81", "C:\\>" },
        { "1978", "bloody\t\tcold" },
	{   "99", "" },
	{ "2002", "libpqxx" },
	{ "1989", "Ode an die Freiheit" },
	{ "2001", "New millennium" },
	{   "97", "Asian crisis" },
	{ "2001", "A Space Odyssey" },
        {0,0}
      };

      cout << "Writing data to " << TableName << endl;

      // Insert tuple of data using "begin" and "end" abstraction
      for (int i=0; CData[i][0]; ++i) 
	W.insert(&CData[i][0], &CData[i][2]);

      // Insert tuple of data held in container
      vector<string> MoreData;
      MoreData.push_back("10");
      MoreData.push_back("Odyssey Two");
      W.insert(MoreData);

      // Now that MoreData has been inserted, we can get rid of the original
      // and use it for something else.  And this time, we use the insertion
      // operator.
      MoreData[0] = "3001";
      MoreData[1] = "Final Odyssey";
      W << MoreData;

      // (destruction of W occurs here)
    }

    // Now that our tablewriter is closed, it's safe to commit T.
    T.commit();
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

