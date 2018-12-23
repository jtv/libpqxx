#include <functional>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of transactor.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
//
// Note for the superstitious: the numbering for this test program is pure
// coincidence.
namespace
{

// Let's take a boring year that is not going to be in the "pqxxevents" table
const unsigned int BoringYear = 1977;


#include <pqxx/internal/ignore-deprecated-pre.hxx>


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
pair<int, int> count_events(connection_base &conn, string table)
{
  int all_years, boring_year;

  const string CountQuery = "SELECT count(*) FROM " + table;

  work tx{conn};
  row R;

  R = tx.exec1(CountQuery);
  R.front().to(all_years);

  R = tx.exec1(CountQuery + " WHERE year=" + to_string(BoringYear));
  R.front().to(boring_year);

  return std::make_pair(all_years, boring_year);
};


struct deliberate_error : exception
{
};


class FailedInsert : public transactor<>
{
  string m_table;
public:
  explicit FailedInsert(string Table) :
    transactor<>("FailedInsert"),
    m_table(Table)
  {
  }

  void operator()(argument_type &T)
  {
    result R = T.exec0(
	"INSERT INTO " + m_table + " VALUES (" +
	to_string(BoringYear) + ", "
	"'yawn')");

    PQXX_CHECK_EQUAL(R.affected_rows(), 1u, "Bad affected_rows().");

    throw deliberate_error();
  }

  void on_abort(const char Reason[]) noexcept
  {
    pqxx::test::expected_exception(name() + " failed: " + Reason);
  }
};


void test_013(transaction_base &T)
{
  connection_base &C{T.conn()};
  T.abort();

  {
    work T2(C);
    test::create_pqxxevents(T2);
    T2.commit();
  }

  const string Table = "pqxxevents";

  const pair<int,int> Before = perform(bind(count_events, ref(C), Table));
  PQXX_CHECK_EQUAL(
	Before.second,
	0,
	"Already have event for " + to_string(BoringYear) + "--can't test.");

  const FailedInsert DoomedTransaction(Table);
  quiet_errorhandler d(C);
  PQXX_CHECK_THROWS(
	C.perform(DoomedTransaction),
	deliberate_error,
	"Failing transactor failed to throw correct exception.");

  const pair<int,int> After = perform(bind(count_events, ref(C), Table));

  PQXX_CHECK_EQUAL(
	After.first,
	Before.first,
	"abort() didn't reset event count.");

  PQXX_CHECK_EQUAL(
	After.second,
	Before.second,
	"abort() didn't reset event count for " + to_string(BoringYear));
}


#include <pqxx/internal/ignore-deprecated-post.hxx>


} // namespace

PQXX_REGISTER_TEST_T(test_013, nontransaction)
