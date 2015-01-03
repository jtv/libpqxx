#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Create a table of numbers, write data to it using
// a tablewriter back_insert_iterator, then verify the table's contents using
// field iterators
namespace
{
void test_083(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string Table = "pqxxnumbers";

  items<items<int> > contents;
  for (int x=0; x<10; ++x)
  {
    items<int> n(x+1);
    contents.push_back(n);
  }

  try
  {
    nontransaction Drop(C, "drop_" + Table);
    quiet_errorhandler d(C);
    Drop.exec("DROP TABLE " + Table);
  }
  catch (const exception &e)
  {
    pqxx::test::expected_exception(string("Could not drop table: ") + e.what());
  }

  work T(C, "test83");
  T.exec("CREATE TEMP TABLE " + Table + "(num INTEGER)");

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
