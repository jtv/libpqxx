#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/robusttransaction>
#include <pqxx/transactor>

#include "helpers.hxx"


// Test and example program for libpqxx.  Verify abort behaviour of
// robusttransaction.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change to roll it back.
namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
constexpr int boring_year_18{1977};


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
std::pair<int, int>
count_events_18(pqxx::connection &cx, std::string const &table)
{
  pqxx::nontransaction tx{cx};
  std::string const count_query{"SELECT count(*) FROM " + table};
  return std::make_pair(
    tx.query_value<int>(count_query),
    tx.query_value<int>(
      count_query + " WHERE year=" + pqxx::to_string(boring_year_18)));
}


void test_018(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::work tx{cx};
    pqxx::test::create_pqxxevents(tx);
    tx.commit();
  }

  std::string const table{"pqxxevents"};

  auto const before{
    pqxx::perform([&cx, &table] { return count_events_18(cx, table); })};
  PQXX_CHECK_EQUAL(
    before.second, 0,
    "Already have event for " + pqxx::to_string(boring_year_18) +
      ", cannot run.");

  {
    PQXX_CHECK_THROWS(
      pqxx::perform([&cx, table] {
        pqxx::robusttransaction<pqxx::serializable> tx{cx};
        tx.exec(
            "INSERT INTO " + table + " VALUES (" +
            pqxx::to_string(boring_year_18) + ", '" + tx.esc("yawn") + "')")
          .no_rows();

        throw pqxx::test::deliberate_error();
      }),
      pqxx::test::deliberate_error);
  }

  auto const after{
    pqxx::perform([&cx, &table] { return count_events_18(cx, table); })};

  PQXX_CHECK_EQUAL(after.first, before.first, "Event count changed.");
  PQXX_CHECK_EQUAL(
    after.second, before.second,
    "Event count for " + pqxx::to_string(boring_year_18) + " changed.");
}


PQXX_REGISTER_TEST(test_018);
} // namespace
