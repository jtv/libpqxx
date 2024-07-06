#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/robusttransaction>
#include <pqxx/transactor>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Verify abort behaviour of RobustTransaction.
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
constexpr long BoringYear{1977};


// Count events and specifically events occurring in Boring Year, leaving the
// former count in the result pair's first member, and the latter in second.
std::pair<int, int> count_events(connection &cx, std::string const &table)
{
  nontransaction tx{cx};
  std::string const count_query{"SELECT count(*) FROM " + table};
  return std::make_pair(
    tx.query_value<int>(count_query),
    tx.query_value<int>(count_query + " WHERE year=" + to_string(BoringYear)));
}


struct deliberate_error : std::exception
{};


void test_018()
{
  connection cx;
  {
    work tx{cx};
    test::create_pqxxevents(tx);
    tx.commit();
  }

  std::string const Table{"pqxxevents"};

  auto const Before{
    perform([&cx, &Table] { return count_events(cx, Table); })};
  PQXX_CHECK_EQUAL(
    Before.second, 0,
    "Already have event for " + to_string(BoringYear) + ", cannot run.");

  {
    quiet_errorhandler d{cx};
    PQXX_CHECK_THROWS(
      perform([&cx, Table] {
        robusttransaction<serializable> tx{cx};
        tx.exec0(
          "INSERT INTO " + Table + " VALUES (" + to_string(BoringYear) +
          ", '" + tx.esc("yawn") + "')");

        throw deliberate_error();
      }),
      deliberate_error,
      "Not getting expected exception from failing transactor.");
  }

  auto const After{perform([&cx, &Table] { return count_events(cx, Table); })};

  PQXX_CHECK_EQUAL(After.first, Before.first, "Event count changed.");
  PQXX_CHECK_EQUAL(
    After.second, Before.second,
    "Event count for " + to_string(BoringYear) + " changed.");
}


PQXX_REGISTER_TEST(test_018);
} // namespace
