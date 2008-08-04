#include <iostream>
#include <vector>

#include <pqxx/connection>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Write a predetermined data set to a table
// using a tablewriter.  This data will be used by subsequent tests.  Any data
// previously in the table will be deleted.

namespace
{

void test_005(connection_base &, transaction_base &T)
{
  string TableName("pqxxorgevents");

  // First create a separate transaction to drop old table, if any.  This may
  // fail if the table didn't previously exist.
  T.exec("DROP TABLE IF EXISTS " + TableName);

  T.exec(("CREATE TABLE " +
	    TableName +
	    "(year INTEGER, event VARCHAR)").c_str());

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
    { "1974", "" },
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

  W.complete();

  // Now that our tablewriter is done, it's safe to commit T.
  T.commit();
}

} // namespace


int main()
{
  test::TestCase<> test005("test_005", test_005);
  return test::pqxxtest(test005);
}

