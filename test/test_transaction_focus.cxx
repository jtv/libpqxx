#include <pqxx/stream_from>
#include <pqxx/stream_to>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
auto make_focus(pqxx::dbtransaction &tx)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  return pqxx::stream_from::query(tx, "SELECT * from generate_series(1, 10)");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_cannot_run_statement_during_focus(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  tx.exec("SELECT 1");
  auto focus{make_focus(tx)};
  PQXX_CHECK_THROWS(
    tx.exec("SELECT 1"), pqxx::usage_error,
    "Command during focus did not throw expected error.");
}


void test_cannot_run_prepared_statement_during_focus(pqxx::test::context &)
{
  pqxx::connection cx;
  cx.prepare("foo", "SELECT 1");
  pqxx::transaction tx{cx};
  tx.exec(pqxx::prepped{"foo"});
  auto focus{make_focus(tx)};
  PQXX_CHECK_THROWS(
    tx.exec(pqxx::prepped{"foo"}), pqxx::usage_error,
    "Prepared statement during focus did not throw expected error.");
}

void test_cannot_run_params_statement_during_focus(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  tx.exec("select $1", pqxx::params{tx, 10});
  auto focus{make_focus(tx)};
  PQXX_CHECK_THROWS(
    tx.exec("select $1", pqxx::params{tx, 10}), pqxx::usage_error,
    "Parameterized statement during focus did not throw expected error.");
}


void test_should_not_end_transaction_before_focus(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  tx.exec("CREATE TEMP TABLE foo(a integer)");
  auto stream{pqxx::stream_to::table(tx, {"foo"}, {"a"})};
  stream.write_values(1);
  // Fail to complete() the stream...
  PQXX_CHECK_THROWS(tx.commit(), pqxx::failure);
}


PQXX_REGISTER_TEST(test_cannot_run_statement_during_focus);
PQXX_REGISTER_TEST(test_cannot_run_prepared_statement_during_focus);
PQXX_REGISTER_TEST(test_cannot_run_params_statement_during_focus);
PQXX_REGISTER_TEST(test_should_not_end_transaction_before_focus);
} // namespace
