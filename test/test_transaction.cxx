#include <pqxx/nontransaction>
#include <pqxx/robusttransaction>

#include "helpers.hxx"


namespace
{
void test_nontransaction_continues_after_error(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::nontransaction tx{cx};

  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 9"), 9);
  PQXX_CHECK_THROWS(tx.exec("SELECT 1/0"), pqxx::sql_error);

  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 5"), 5);
}


/// A temporary table for counting things.  Basically a global int in the DB.
/** The table gets a randomised name.  The destructor does not clean it up;
 * let that happen automatically.
 *
 * Does not remember the transaction.  Feel free to use a different one on
 * every call.
 *
 * To create a counter that survives across transactions, create it in a
 * `pqxx::nontransaction`.
 */
class counter_table final
{
public:
  /// Create the table, initialise counter to zero.
  counter_table(
    pqxx::test::context &tctx, pqxx::transaction_base &tx,
    pqxx::sl loc = pqxx::sl::current()) :
          m_name{tctx.make_name("pqxx")}
  {
    tx.exec(std::format("CREATE TEMP TABLE {} (n int)", m_name), loc)
      .no_rows(loc);
    tx.exec(std::format("INSERT INTO {}(n) VALUES ({})", m_name, 0), loc)
      .no_rows(loc);
  }

  /// Increment the counter by 1.
  void
  increment(pqxx::transaction_base &tx, pqxx::sl loc = pqxx::sl::current())
  {
    tx.exec(std::format("UPDATE {} SET n = n + 1", m_name), loc).no_rows();
  }

  [[nodiscard]] int
  get(pqxx::transaction_base &tx, pqxx::sl loc = pqxx::sl::current()) const
  {
    return tx.query_value<int>(std::format("SELECT n FROM {}", m_name), loc);
  }

private:
  std::string m_name;
};


void test_nontransaction_autocommits(pqxx::test::context &tctx)
{
  pqxx::connection cx;

  pqxx::nontransaction tx1{cx};
  counter_table table{tctx, tx1};
  tx1.commit();

  pqxx::nontransaction tx2{cx};
  table.increment(tx2);
  tx2.abort();

  pqxx::nontransaction tx3{cx};
  PQXX_CHECK_EQUAL(table.get(tx3), 1);
}


template<typename TX> void test_double_close(pqxx::test::context &)
{
  pqxx::connection cx;

  // You can re-commit a committed transaction.  It does nothing.
  TX tx1{cx};
  tx1.exec("SELECT 1").one_row();
  tx1.commit();
  tx1.commit();

  // You can re-abort an aborted transaction.  It does nothing.
  TX tx2{cx};
  tx2.exec("SELECT 2").one_row();
  tx2.abort();
  tx2.abort();

  // You can not abort a committed transaction.
  TX tx3{cx};
  tx3.exec("SELECT 3").one_row();
  tx3.commit();
  PQXX_CHECK_THROWS(tx3.abort(), pqxx::usage_error);

  // You can not commit an aborted transaction.
  TX tx4{cx};
  tx4.exec("SELECT 4").one_row();
  tx4.abort();
  PQXX_CHECK_THROWS(tx4.commit(), pqxx::usage_error);
}


template<typename TX> void test_failed_commit(pqxx::test::context &)
{
  pqxx::connection cx;
  TX tx{cx};

  tx.exec("CREATE TEMP TABLE foo (id integer UNIQUE INITIALLY DEFERRED)");
  tx.exec("INSERT INTO foo VALUES (1), (1)");

  // Database checks the unique constraint (and fails it) at commit.
  PQXX_CHECK_THROWS(tx.commit(), pqxx::unique_violation);

  // Repeated attempt to commit fails because the transaction aborted.
  PQXX_CHECK_THROWS(tx.commit(), pqxx::usage_error);

  // Repeated abort does nothing.
  tx.abort();
  tx.abort();
}


template<typename TX>
void test_commit_on_broken_connection(pqxx::test::context &)
{
  pqxx::connection cx;
  TX tx{cx};
  cx.close();
  PQXX_CHECK_THROWS(tx.commit(), pqxx::broken_connection);
}


void test_transaction(pqxx::test::context &tctx)
{
  // TODO: Overhaul test harness so we can register these directly.
  test_double_close<pqxx::transaction<>>(tctx);
  test_double_close<pqxx::read_transaction>(tctx);
  test_double_close<pqxx::nontransaction>(tctx);
  test_double_close<pqxx::robusttransaction<>>(tctx);
  test_failed_commit<pqxx::transaction<>>(tctx);
  test_failed_commit<pqxx::robusttransaction<>>(tctx);
  test_commit_on_broken_connection<pqxx::transaction<>>(tctx);
  test_commit_on_broken_connection<pqxx::robusttransaction<>>(tctx);
}


void test_compound_statement_returns_last_result(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const output{tctx.make_num()};
  auto const res{tx.exec(
    std::format(
      "SELECT {}, {} ; SELECT {}", tctx.make_num(), tctx.make_num(), output))};
  PQXX_CHECK_EQUAL(std::size(res), 1);
  PQXX_CHECK_EQUAL(std::size(res.at(0)), 1);
  PQXX_CHECK_EQUAL(res.one_row().at(0).as<int>(), output);
}


void test_compound_statement_throws_first_error(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const testtab{tctx.make_name("pqxx")};
  tx.exec(std::format("CREATE TEMP TABLE {} (n integer)", testtab)).no_rows();

  // Unparameterised statement, different code path.
  PQXX_CHECK_THROWS(
    tx.exec(
      std::format(
        "SELECT {} ; SELECT x FROM {} ; SELECT 0 / 0 ; SELECT {}",
        tctx.make_num(), testtab, tctx.make_num())),
    pqxx::undefined_column);
}


PQXX_REGISTER_TEST(test_nontransaction_continues_after_error);
PQXX_REGISTER_TEST(test_nontransaction_autocommits);
PQXX_REGISTER_TEST(test_transaction);
PQXX_REGISTER_TEST(test_compound_statement_returns_last_result);
PQXX_REGISTER_TEST(test_compound_statement_throws_first_error);
} // namespace
