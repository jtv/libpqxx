#include <pqxx/transaction>
#include <pqxx/transactor>

#include "helpers.hxx"


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
constexpr int boring_year_13 = 1977;


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
std::pair<int, int>
count_events_13(pqxx::connection &cx, std::string const &table)
{
  pqxx::work tx{cx};
  std::string const count_query{"SELECT count(*) FROM " + table};
  return std::make_pair(
    tx.query_value<int>(count_query),
    tx.query_value<int>(
      count_query + " WHERE year=" + pqxx::to_string(boring_year_13)));
}


void failed_insert(pqxx::connection &cx, std::string const &table)
{
  pqxx::work tx(cx);
  pqxx::result const R = tx.exec(
                             "INSERT INTO " + table + " VALUES (" +
                             pqxx::to_string(boring_year_13) +
                             ", "
                             "'yawn')")
                           .no_rows();

  PQXX_CHECK_EQUAL(R.affected_rows(), 1);
  throw pqxx::test::deliberate_error();
}


void test_013(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::work tx{cx};
    pqxx::test::create_pqxxevents(tx);
    tx.commit();
  }

  std::string const Table{"pqxxevents"};

  auto const Before{
    pqxx::perform([&cx, &Table] { return count_events_13(cx, Table); })};
  PQXX_CHECK_EQUAL(
    Before.second, 0,
    "Already have event for " + pqxx::to_string(boring_year_13) +
      "--can't test.");

#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::quiet_errorhandler const d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_THROWS(
    pqxx::perform([&cx, &Table] { failed_insert(cx, Table); }),
    pqxx::test::deliberate_error);

  auto const After{
    pqxx::perform([&cx, &Table] { return count_events_13(cx, Table); })};

  PQXX_CHECK_EQUAL(
    After.first, Before.first, "abort() didn't reset event count.");

  PQXX_CHECK_EQUAL(
    After.second, Before.second,
    "abort() didn't reset event count for " + pqxx::to_string(boring_year_13));
}


PQXX_REGISTER_TEST(test_013);
} // namespace
