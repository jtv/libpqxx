#include <pqxx/nontransaction>

#include "helpers.hxx"


// Test program for libpqxx.  Open connection to database, start a transaction,
// abort it, and verify that it "never happened" (due to the rollback).

namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
constexpr int boring_year_10{1977};

std::string_view const table("pqxxevents");


// Count events, and boring events, in table
std::pair<int, int> count_events(pqxx::transaction_base &T)
{
  std::string const events_query{
    std::format("SELECT count(*) FROM {}", table)};
  std::string const boring_query{
    events_query + " WHERE year=" + pqxx::to_string(boring_year_10)};
  return std::make_pair(
    T.query_value<int>(events_query), T.query_value<int>(boring_query));
}


// Try adding a record, then aborting it, and check whether the abort was
// performed correctly.
void Test(pqxx::connection &C, bool ExplicitAbort)
{
  std::pair<int, int> event_counts;

  // First run our doomed transaction.  This will refuse to run if an event
  // exists for our Boring Year.
  {
    // Begin a transaction acting on our current connection; we'll abort it
    // later though.
    pqxx::work doomed{C, "doomed"};

    // Verify that our Boring Year was not yet in the events table
    event_counts = count_events(doomed);

    PQXX_CHECK_EQUAL(
      event_counts.second, 0, "Can't run, boring year is already in table.");

    // Now let's try to introduce a row for our Boring Year
    doomed
      .exec(
        std::format("INSERT INTO {} (year, event) ", table) + "VALUES (" +
        pqxx::to_string(boring_year_10) + ", 'yawn')")
      .no_rows();

    auto const Recount{count_events(doomed)};
    PQXX_CHECK_EQUAL(Recount.second, 1);
    PQXX_CHECK_EQUAL(Recount.first, event_counts.first + 1);

    // Okay, we've added an entry but we don't really want to.  Abort it
    // explicitly if requested, or simply let the Transaction object "expire."
    if (ExplicitAbort)
      doomed.abort();

    // The doomed tranaction ends here because it goes out of scope, even if
    // there was no explicit commit or abort.
  }

  // Now check that we're back in the original state.  Note that this may go
  // wrong if somebody managed to change the table between our two
  // transactions.
  pqxx::work checkup(C, "checkup");

  auto const NewEvents{count_events(checkup)};
  PQXX_CHECK_EQUAL(NewEvents.first, event_counts.first);

  PQXX_CHECK_EQUAL(NewEvents.second, 0);
}


void test_abort(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::nontransaction tx{cx};
  pqxx::test::create_pqxxevents(tx);
  pqxx::connection &c(tx.conn());
  tx.commit();
  Test(c, true);
  Test(c, false);
}


PQXX_REGISTER_TEST(test_abort);
} // namespace
