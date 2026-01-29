#include <pqxx/nontransaction>
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
constexpr int boring_year_32{1977};

std::pair<int, int>
count_events_32(pqxx::connection &cx, std::string const &table)
{
  std::string const count_query{"SELECT count(*) FROM " + table};
  pqxx::work tx{cx};
  return std::make_pair(
    tx.query_value<int>(count_query),
    tx.query_value<int>(
      count_query + " WHERE year=" + pqxx::to_string(boring_year_32)));
}


void test_032(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::nontransaction tx{cx};
    pqxx::test::create_pqxxevents(tx);
  }

  std::string const table{"pqxxevents"};

  std::pair<int, int> const before{
    pqxx::perform([&cx, &table] { return count_events_32(cx, table); })};
  PQXX_CHECK_EQUAL(
    before.second, 0,
    "Already have event for " + pqxx::to_string(boring_year_32) +
      ", cannot test.");

  {
#include "pqxx/internal/ignore-deprecated-pre.hxx"
    pqxx::quiet_errorhandler const d(cx);
#include "pqxx/internal/ignore-deprecated-post.hxx"
    PQXX_CHECK_THROWS(
      pqxx::perform([&cx, &table] {
        pqxx::work{cx}
          .exec(
            "INSERT INTO " + table + " VALUES (" +
            pqxx::to_string(boring_year_32) +
            ", "
            "'yawn')")
          .no_rows();
        throw pqxx::test::deliberate_error();
      }),
      pqxx::test::deliberate_error);
  }

  std::pair<int, int> const after{
    pqxx::perform([&cx, &table] { return count_events_32(cx, table); })};

  PQXX_CHECK_EQUAL(after.first, before.first);
  PQXX_CHECK_EQUAL(after.second, before.second);
}


PQXX_REGISTER_TEST(test_032);
} // namespace
