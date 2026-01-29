#include <pqxx/nontransaction>
#include <pqxx/robusttransaction>
#include <pqxx/transactor>

#include "helpers.hxx"


// Test program for libpqxx.  Verify abort behaviour of RobustTransaction.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
constexpr int boring_year_37{1977};

// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
std::pair<int, int>
count_events_37(pqxx::connection &cx, std::string const &table)
{
  std::string const count_query{"SELECT count(*) FROM " + table};
  pqxx::nontransaction tx{cx};
  return std::make_pair(
    tx.query_value<int>(count_query),
    tx.query_value<int>(
      count_query + " WHERE year=" + pqxx::to_string(boring_year_37)));
}


void test_037(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::nontransaction tx{cx};
    pqxx::test::create_pqxxevents(tx);
  }

  std::string const table{"pqxxevents"};

  auto const Before{
    pqxx::perform([&cx, &table] { return count_events_37(cx, table); })};
  PQXX_CHECK_EQUAL(
    Before.second, 0,
    "Already have event for " + pqxx::to_string(boring_year_37) +
      ", cannot test.");

  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_THROWS(
      pqxx::perform([&cx, &table] {
        pqxx::robusttransaction<> tx{cx};
        tx.exec(
            "INSERT INTO " + table + " VALUES (" +
            pqxx::to_string(boring_year_37) +
            ", "
            "'yawn')")
          .no_rows();

        throw pqxx::test::deliberate_error();
      }),
      pqxx::test::deliberate_error);
  }

  auto const After{
    pqxx::perform([&cx, &table] { return count_events_37(cx, table); })};

  PQXX_CHECK_EQUAL(After.first, Before.first);
  PQXX_CHECK_EQUAL(After.second, Before.second);
}


PQXX_REGISTER_TEST(test_037);
} // namespace
