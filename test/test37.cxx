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
constexpr int BoringYear{1977};

// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
std::pair<int, int>
count_events(pqxx::connection &cx, std::string const &table)
{
  std::string const count_query{"SELECT count(*) FROM " + table};
  pqxx::nontransaction tx{cx};
  return std::make_pair(
    tx.query_value<int>(count_query),
    tx.query_value<int>(
      count_query + " WHERE year=" + pqxx::to_string(BoringYear)));
}


struct deliberate_error : std::exception
{};


void test_037()
{
  pqxx::connection cx;
  {
    pqxx::nontransaction tx{cx};
    pqxx::test::create_pqxxevents(tx);
  }

  std::string const Table{"pqxxevents"};

  auto const Before{
    pqxx::perform([&cx, &Table] { return count_events(cx, Table); })};
  PQXX_CHECK_EQUAL(
    Before.second, 0,
    "Already have event for " + pqxx::to_string(BoringYear) +
      ", cannot test.");

  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_THROWS(
      pqxx::perform([&cx, &Table] {
        pqxx::robusttransaction<> tx{cx};
        tx.exec(
            "INSERT INTO " + Table + " VALUES (" +
            pqxx::to_string(BoringYear) +
            ", "
            "'yawn')")
          .no_rows();

        throw deliberate_error();
      }),
      deliberate_error,
      "Did not get expected exception from failing transactor.");
  }

  auto const After{
    pqxx::perform([&cx, &Table] { return count_events(cx, Table); })};

  PQXX_CHECK_EQUAL(After.first, Before.first, "Number of events changed.");
  PQXX_CHECK_EQUAL(
    After.second, Before.second,
    "Number of events for " + pqxx::to_string(BoringYear) + " changed.");
}


PQXX_REGISTER_TEST(test_037);
} // namespace
