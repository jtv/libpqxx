#include <iostream>
#include <vector>

#include <pqxx/connection.h>
#include <pqxx/tablewriter.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Write a predetermined data set to a table
// using a TableWriter.  This data will be used by subsequent tests.  Any data
// previously in the table will be deleted.
//
// Usage: test5 [connect-string] [tablename]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
//
// The tablename argument determines which table the data will be written to.
// If none is given, it defaults to "orgevents".
int main(int argc, char *argv[])
{
  try
  {
    // Set up a connection to the backend
    Connection C(argv[1] ? argv[1] : "");

    string TableName((argc > 2) ? argv[2] : "orgevents");

    // First create a separate transaction to drop old table, if any.  This may
    // fail if the table didn't previously exist.
    cout << "Dropping old " << TableName << endl;
    try
    {
      Transaction Drop(C, "drop_" + TableName);
      Drop.Exec(("DROP TABLE " + TableName).c_str());
      Drop.Commit();
    }
    catch (const exception &e)
    {
      cerr << "(Expected) Couldn't drop table: " << e.what() << endl;
    }

    // Now begin new transaction to create new table & write data
    Transaction T(C, "test5");

    T.Exec(("CREATE TABLE " + 
	    TableName + 
	    "(year INTEGER, event VARCHAR)").c_str());

    // NOTE: start a nested block here to ensure that our stream W is closed 
    // before we attempt to commit our transaction T.  Otherwise we might end 
    // up committing T before all data going into W had been written.
    {
      TableWriter W(T, TableName);

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
	{ "2001", "A Space Oddyssey" },
        {0,0}
      };

      cout << "Writing data to " << TableName << endl;

      // Insert tuple of data using "begin" and "end" abstraction
      for (int i=0; CData[i][0]; ++i) 
	W.insert(&CData[i][0], &CData[i][2]);

      // Insert tuple of data held in container
      vector<string> MoreData;
      MoreData.push_back("10");
      MoreData.push_back("Oddyssey Two");
      W.insert(MoreData);

      // Now that MoreData has been inserted, we can get rid of the original
      // and use it for something else.  And this time, we use the insertion
      // operator.
      MoreData[0] = "3001";
      MoreData[1] = "Final Oddyssey";
      W << MoreData;

      // (destruction of W occurs here)
    }

    // Now that our TableWriter is closed, it's safe to commit T.
    T.Commit();
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

