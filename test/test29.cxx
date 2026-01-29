#include <vector>

#include <pqxx/nontransaction>
#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Open connection to database, start a transaction,
// abort it, and verify that it "never happened."
//
// The program will attempt to add an entry to a table called "pqxxevents",
// with a key column called "year"--and then abort the change.
namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
constexpr int boring_year_29{1977};

std::string_view const table{"pqxxevents"};


// Count events, and boring events, in table
std::pair<int, int> count_events(pqxx::transaction_base &tx)
{
  std::string const events_query{
    std::format("SELECT count(*) FROM {}", table)};
  std::string const boring_query{
    events_query + " WHERE year=" + pqxx::to_string(boring_year_29)};

  return std::make_pair(
    tx.query_value<int>(events_query), tx.query_value<int>(boring_query));
}


// Try adding a record, then aborting it, and check whether the abort was
// performed correctly.
void check(pqxx::connection &cx, bool explicit_abort)
{
  std::pair<int, int> event_counts;

  // First run our doomed transaction.  This will refuse to run if an event
  // exists for our Boring Year.
  {
    // Begin a transaction acting on our current connection; we'll abort it
    // later though.
    pqxx::work doomed{cx, "doomed"};

    // Verify that our Boring Year was not yet in the events table
    event_counts = count_events(doomed);

    PQXX_CHECK_EQUAL(
      event_counts.second, 0,
      "Can't run; " + pqxx::to_string(boring_year_29) +
        " is already in the table.");

    // Now let's try to introduce a row for our Boring Year
    doomed
      .exec(
        std::format("INSERT INTO {}", table) +
        "(year, event) "
        "VALUES (" +
        pqxx::to_string(boring_year_29) + ", 'yawn')")
      .no_rows();

    auto recount{count_events(doomed)};
    PQXX_CHECK_EQUAL(recount.second, 1);
    PQXX_CHECK_EQUAL(recount.first, event_counts.first + 1);

    // Okay, we've added an entry but we don't really want to.  Abort it
    // explicitly if requested, or simply let the Transaction object "expire."
    if (explicit_abort)
      doomed.abort();

    // If now explicit abort requested, doomed Transaction still ends here
  }

  // Now check that we're back in the original state.  Note that this may go
  // wrong if somebody managed to change the table between our two
  // transactions.
  pqxx::work checkup{cx, "checkup"};

  auto new_events{count_events(checkup)};
  PQXX_CHECK_EQUAL(new_events.first, event_counts.first);

  PQXX_CHECK_EQUAL(new_events.second, 0);
}


void test_029(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::nontransaction tx{cx};
    pqxx::test::create_pqxxevents(tx);
  }

  // Test abort semantics, both with explicit and implicit abort
  check(cx, true);
  check(cx, false);
}


PQXX_REGISTER_TEST(test_029);
} // namespace
