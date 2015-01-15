#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Attempt to perform nested transactions.
namespace
{
void test_088(transaction_base &T0)
{
  test::create_pqxxevents(T0);
  connection_base &C(T0.conn());

  // Trivial test: create subtransactions, and commit/abort
  cout << T0.exec("SELECT 'T0 starts'")[0][0].c_str() << endl;

  subtransaction T0a(static_cast<dbtransaction &>(T0), "T0a");
  T0a.commit();

  subtransaction T0b(static_cast<dbtransaction &>(T0), "T0b");
  T0b.abort();
  cout << T0.exec("SELECT 'T0 ends'")[0][0].c_str() << endl;
  T0.commit();

  // Basic functionality: perform query in subtransaction; abort, continue
  work T1(C, "T1");
  cout << T1.exec("SELECT 'T1 starts'")[0][0].c_str() << endl;
  subtransaction T1a(T1, "T1a");
    cout << T1a.exec("SELECT '  a'")[0][0].c_str() << endl;
    T1a.commit();
  subtransaction T1b(T1, "T1b");
    cout << T1b.exec("SELECT '  b'")[0][0].c_str() << endl;
    T1b.abort();
  subtransaction T1c(T1, "T1c");
    cout << T1c.exec("SELECT '  c'")[0][0].c_str() << endl;
    T1c.commit();
  cout << T1.exec("SELECT 'T1 ends'")[0][0].c_str() << endl;
  T1.commit();

  // Commit/rollback functionality
  work T2(C, "T2");
  const string Table = "test088";
  T2.exec("CREATE TEMP TABLE " + Table + "(no INTEGER, text VARCHAR)");

  T2.exec("INSERT INTO " + Table + " VALUES(1,'T2')");

  subtransaction T2a(T2, "T2a");
    T2a.exec("INSERT INTO "+Table+" VALUES(2,'T2a')");
    T2a.commit();
  subtransaction T2b(T2, "T2b");
    T2b.exec("INSERT INTO "+Table+" VALUES(3,'T2b')");
    T2b.abort();
  subtransaction T2c(T2, "T2c");
    T2c.exec("INSERT INTO "+Table+" VALUES(4,'T2c')");
    T2c.commit();
  const result R = T2.exec("SELECT * FROM " + Table + " ORDER BY no");
  for (result::const_iterator i=R.begin(); i!=R.end(); ++i)
    cout << '\t' << i[0].c_str() << '\t' << i[1].c_str() << endl;

  PQXX_CHECK_EQUAL(R.size(), 3u, "Wrong number of results.");

  int expected[3] = { 1, 2, 4 };
  for (result::size_type n=0; n<R.size(); ++n)
    PQXX_CHECK_EQUAL(
	R[n][0].as<int>(),
	expected[n], 
	"Hit unexpected row number.");

  T2.abort();

  // Auto-abort should only roll back the subtransaction.
  work T3(C, "T3");
  subtransaction T3a(T3, "T3a");
  PQXX_CHECK_THROWS(
	T3a.exec("SELECT * FROM nonexistent_table WHERE nonattribute=0"),
	sql_error,
	"Bogus query did not fail.");

  // Subtransaction can only be aborted now, because there was an error.
  T3a.abort();
  // We're back in our top-level transaction.  This did not abort.
  T3.exec("SELECT count(*) FROM pqxxevents");
  // Make sure we can commit exactly one more level of transaction.
  T3.commit();
}
} // namespace

PQXX_REGISTER_TEST(test_088)
