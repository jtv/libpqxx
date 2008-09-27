#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Write a predetermined data set to a table using a
// tablewriter on a deferred connection.  This data will be used by subsequent
// tests.  Any data previously in the table will be deleted.
namespace
{
void test_024(connection_base &, transaction_base &)
{
  // Set up a deferred connection to the backend
  lazyconnection C;

  string TableName("pqxxorgevents");

  cout << "Dropping old " << TableName << endl;
  try
  {
    work Drop(C, "drop_" + TableName);
    Drop.exec("DROP TABLE " + TableName);
    Drop.commit();
  }
  catch (const exception &e)
  {
    cerr << "Couldn't drop table: " << e.what() << endl;
  }

  work T(C, "test5");

  T.exec("CREATE TABLE " + TableName + "(year INTEGER, event VARCHAR)");

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
      { "1974", "" },
      {   "97", "Asian crisis" },
      {   "01", "A Space Odyssey" },
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

  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_024)
