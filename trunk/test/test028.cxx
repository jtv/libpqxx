#include <iostream>
#include <set>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Create a table and write data to it, using
// tablewriter's back_insert_iterator, and on a lazy connection.
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
  W.reserve(tablewriter::size_type(Contents.size()));

  copy(Contents.begin(), Contents.end(), back_inserter(W));

  cout << Contents.size() << " rows written." << endl;
}


void CheckTable(transaction_base &T, string TableName)
{
  result Count = T.exec("SELECT COUNT(*) FROM " + TableName);
  size_t Rows = 0;

  PQXX_CHECK(Count[0][0].to(Rows), "Row count is NULL.");
  cout << Rows << " rows in table." << endl;

  PQXX_CHECK_EQUAL(Rows, Contents.size(), "Got different number of rows back.");
  // TODO: Compare table contents to Contents
}


void test_028(connection_base &, transaction_base &T)
{
  PrepareContents();

  // Select our original and destination table names
  const string TableName = "testtable";

  // Create table.  If the table already existed, better to fail now.
  T.exec("CREATE TABLE " + TableName + "(content VARCHAR)");

  FillTable(T, TableName);
  CheckTable(T, TableName);

  T.exec("DROP TABLE " + TableName);
  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_C(test_028, lazyconnection)
