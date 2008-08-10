#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Create a table and write data to it, using
// tablewriter's back_insert_iterator.

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


void FillTable(transaction_base &T, string TableName, string Column)
{
  tablewriter W(T, TableName, &Column, &Column+1);
  W.reserve(Contents.size());

  copy(Contents.begin(), Contents.end(), back_inserter(W));

  cout << Contents.size() << " rows written." << endl;
}


void CheckTable(transaction_base &T, string TableName)
{
  result Count = T.exec(("SELECT COUNT(*) FROM " + TableName).c_str());
  size_t Rows = 0;

  const bool have_rowcount = Count[0][0].to(Rows);
  PQXX_CHECK(have_rowcount, "NULL row count.");
  cout << Rows << " rows in table." << endl;

  PQXX_CHECK_EQUAL(
	Rows,
	Contents.size(),
	"Number of rows in table is not what I wrote.");
  // TODO: Compare table contents to Contents
}

void test_009(connection_base &, transaction_base &T)
{
  PrepareContents();

  // Select our original and destination table names
  const string TableName = "pqxxtesttable";

  const string Column = "content";

  // Create table.  If the table already existed, better to fail now.
  stringstream ctq;
  ctq << "CREATE TABLE " << TableName << '(' << Column << " VARCHAR)";
  T.exec(ctq);

  FillTable(T, TableName, Column);
  CheckTable(T, TableName);

  T.exec(("DROP TABLE " + TableName).c_str());
  T.commit();
}

} // namespace

PQXX_REGISTER_TEST(test_009)
