#include <iostream>
#include <vector>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/tablewriter>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Create a table of numbers, write data to it using
// a tablewriter back_insert_iterator, then verify the table's contents using
// field iterators
namespace
{
void test_083(connection_base &C, transaction_base &orgT)
{
  orgT.abort();

  const string Table = "pqxxnumbers";

  items<items<int> > contents;
  for (int x=0; x<10; ++x)
  {
    items<int> n(x+1);
    contents.push_back(n);
  }

  cout << "Dropping old " << Table << endl;
  try
  {
    nontransaction Drop(C, "drop_" + Table);
    disable_noticer d(C);
    Drop.exec("DROP TABLE " + Table);
  }
  catch (const undefined_table &e)
  {
    cout << "(Expected) Couldn't drop table: " << e.what() << endl
	 << "Query was: " << e.query() << endl;
  }
  catch (const sql_error &e)
  {
    cerr << "Couldn't drop table: " << e.what() << endl
	 << "Query was: " << e.query() << endl;
  }

  work T(C, "test83");
  T.exec("CREATE TABLE " + Table + "(num INTEGER)");

  tablewriter W(T, Table);
  back_insert_iterator<tablewriter> b(W);
  items<items<int> >::const_iterator i = contents.begin();
  *b = *i++;
  ++b;
  *b++ = *i++;
  back_insert_iterator<tablewriter> c(W);
  c = b;
  *c++ = *i;
  W.complete();

  const result R = T.exec("SELECT * FROM " + Table + " ORDER BY num DESC");
  items<items<int> >::const_reverse_iterator j(++i);
  for (result::const_iterator r = R.begin(); r != R.end(); ++j, ++r)
    PQXX_CHECK_EQUAL(
	r->at(0).as(0),
	(*j)[0],
	"Writing numbers with tablewriter went wrong.");

  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_083, nontransaction)
