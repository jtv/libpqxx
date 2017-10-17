#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Create a table of numbers, write data to it using
// a tablewriter back_insert_iterator, then verify the table's contents using
// field iterators
namespace
{
/// Create a vector of vectors of ints: {{1}, {2}, {3}, ...}
std::vector<std::vector<int>> make_contents()
{
  std::vector<std::vector<int>> contents;
  for (int x=0; x<10; ++x) contents.push_back(std::vector<int>{x + 1});
  return contents;
}


void test_083(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string Table = "pqxxnumbers";
  const auto contents = make_contents();
  try
  {
    nontransaction Drop(C, "drop_" + Table);
    quiet_errorhandler d(C);
    Drop.exec0("DROP TABLE " + Table);
  }
  catch (const exception &e)
  {
    pqxx::test::expected_exception(string("Could not drop table: ") + e.what());
  }

  work T(C, "test83");
  T.exec0("CREATE TEMP TABLE " + Table + "(num INTEGER)");

  tablewriter W(T, Table);
  back_insert_iterator<tablewriter> b(W);
  auto i = contents.begin();
  *b = *i++;
  ++b;
  *b++ = *i++;
  back_insert_iterator<tablewriter> c(W);
  c = b;
  *c++ = *i;
  W.complete();

  const result R = T.exec("SELECT * FROM " + Table + " ORDER BY num DESC");
  decltype(contents)::const_reverse_iterator j(++i);
  for (const auto &r: R)
  {
    PQXX_CHECK_EQUAL(
	r.at(0).as(0),
	(*j)[0],
	"Writing numbers with tablewriter went wrong.");
    ++j;
  }

  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_083, nontransaction)
