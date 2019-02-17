#include <functional>

#include "test_helpers.hxx"

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


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
std::pair<int, int> count_events(connection_base &conn, std::string table)
{
  int all_years, boring_year;

  const std::string CountQuery = "SELECT count(*) FROM " + table;

  work tx{conn};
  row R;

  R = tx.exec1(CountQuery);
  R.front().to(all_years);

  R = tx.exec1(CountQuery + " WHERE year=" + to_string(BoringYear));
  R.front().to(boring_year);

  return std::make_pair(all_years, boring_year);
}


struct deliberate_error : std::exception
{
};


void failed_insert(connection_base &C, std::string table)
{
  work tx(C);
  result R = tx.exec0(
	"INSERT INTO " + table + " VALUES (" +
	to_string(BoringYear) + ", "
	"'yawn')");

  PQXX_CHECK_EQUAL(R.affected_rows(), 1u, "Bad affected_rows().");
  throw deliberate_error();
}


void test_013()
{
  connection conn;
  {
    work tx{conn};
    test::create_pqxxevents(tx);
    tx.commit();
  }

  const std::string Table = "pqxxevents";

  const std::pair<int,int> Before = perform(
	std::bind(count_events, std::ref(conn), Table));
  PQXX_CHECK_EQUAL(
	Before.second,
	0,
	"Already have event for " + to_string(BoringYear) + "--can't test.");

  quiet_errorhandler d(conn);
  PQXX_CHECK_THROWS(
	perform(std::bind(failed_insert,  std::ref(conn), Table)),
	deliberate_error,
	"Failing transactor failed to throw correct exception.");

  const std::pair<int,int> After = perform(
	std::bind(count_events, std::ref(conn), Table));

  PQXX_CHECK_EQUAL(
	After.first,
	Before.first,
	"abort() didn't reset event count.");

  PQXX_CHECK_EQUAL(
	After.second,
	Before.second,
	"abort() didn't reset event count for " + to_string(BoringYear));
}


PQXX_REGISTER_TEST(test_013);
} // namespace
