#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Create a table and write data to it, using
// tablewriter's back_insert_iterator, and on a lazy connection.
//
// Usage: test028 [connect-string] [table]
//
// Where the connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend 
// running on host foo.bar.net, logging in as user smith.
//
// The default table name is "testtable."

namespace
{
set< vector<string> > Contents;


void PrepareContents()
{
  const char *Strings[] =
  {
    "foo",
    "bar",
    "!",
    "\t",
    "'",
    "\"",
    " ",
    "|",
    "*",

    0
  };

  for (int i=0; Strings[i]; ++i)
  {
    vector<string> v;
    v.push_back(Strings[i]);
    Contents.insert(v);
  }
}


void FillTable(transaction_base &T, string TableName)
{
  tablewriter W(T, TableName);
  W.reserve(Contents.size());

  copy(Contents.begin(), Contents.end(), back_inserter(W));

  cout << Contents.size() << " rows written." << endl;
}


void CheckTable(transaction_base &T, string TableName)
{
  result Count = T.exec("SELECT COUNT(*) FROM " + TableName);
  size_t Rows = 0;

  if (!Count[0][0].to(Rows)) throw runtime_error("NULL row count!");
  cout << Rows << " rows in table." << endl;

  if (Rows != Contents.size())
    throw runtime_error("Found " + 
		        string(Count[0][0].c_str()) + 
			" rows in table--after writing " +
			ToString(Contents.size()) +
			"!");

  // TODO: Compare table contents to Contents
}
}

int main(int argc, char *argv[])
{
  try
  {
    PrepareContents();

    // Set up two connections to the backend: one to read our original table,
    // and another to write our copy
    lazyconnection C(argv[1]);

    // Select our original and destination table names
    string TableName = "testtable";
    if (argc > 2) TableName = argv[2];

    work T(C, "test28");

    // Create table.  If the table already existed, better to fail now.
    T.exec("CREATE TABLE " + TableName + "(content VARCHAR)");

    FillTable(T, TableName);
    CheckTable(T, TableName);

    T.exec("DROP TABLE " + TableName);
    T.commit();
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

