#include <pqxx/nontransaction>

#include "helpers.hxx"


// Test program for libpqxx.  Open connection to database, start a transaction,
// abort it, and verify that it "never happened."

namespace
{
// Let's take a boring year that is not going to be in the "pqxxevents" table
constexpr int boring_year_10{1977};

std::string_view const Table("pqxxevents");


// Count events, and boring events, in table
std::pair<int, int> CountEvents(pqxx::transaction_base &T)
{
  std::string const events_query{
    std::format("SELECT count(*) FROM {}", Table)};
  std::string const boring_query{
    events_query + " WHERE year=" + pqxx::to_string(boring_year_10)};
  return std::make_pair(
    T.query_value<int>(events_query), T.query_value<int>(boring_query));
}


// Try adding a record, then aborting it, and check whether the abort was
// performed correctly.
void Test(pqxx::connection &C, bool ExplicitAbort)
{
  std::pair<int, int> EventCounts;

  // First run our doomed transaction.  This will refuse to run if an event
  // exists for our Boring Year.
  {
    // Begin a transaction acting on our current connection; we'll abort it
    // later though.
    pqxx::work Doomed{C, "Doomed"};

    // Verify that our Boring Year was not yet in the events table
    EventCounts = CountEvents(Doomed);

    PQXX_CHECK_EQUAL(
      EventCounts.second, 0, "Can't run, boring year is already in table.");

    // Now let's try to introduce a row for our Boring Year
    Doomed
      .exec(
        std::format("INSERT INTO {} (year, event) ", Table) + "VALUES (" +
        pqxx::to_string(boring_year_10) + ", 'yawn')")
      .no_rows();

    auto const Recount{CountEvents(Doomed)};
    PQXX_CHECK_EQUAL(Recount.second, 1);
    PQXX_CHECK_EQUAL(Recount.first, EventCounts.first + 1);

    // Okay, we've added an entry but we don't really want to.  Abort it
    // explicitly if requested, or simply let the Transaction object "expire."
    if (ExplicitAbort)
      Doomed.abort();

    // If now explicit abort requested, Doomed Transaction still ends here
  }

  // Now check that we're back in the original state.  Note that this may go
  // wrong if somebody managed to change the table between our two
  // transactions.
  pqxx::work Checkup(C, "Checkup");

  auto const NewEvents{CountEvents(Checkup)};
  PQXX_CHECK_EQUAL(NewEvents.first, EventCounts.first);

  PQXX_CHECK_EQUAL(NewEvents.second, 0);
}


void test_abort()
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
