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


std::string_view const table{"pqxx_test_transaction"};


void delete_temp_table(pqxx::transaction_base &tx)
{
  tx.exec(std::format("DROP TABLE IF EXISTS {}", table)).no_rows();
}


void create_temp_table(pqxx::transaction_base &tx)
{
  tx.exec(std::format("CREATE TEMP TABLE {} (x integer)", table)).no_rows();
}


void insert_temp_table(pqxx::transaction_base &tx, int value)
{
  tx.exec(
      std::format("INSERT INTO {} (x) VALUES (", table) +
      pqxx::to_string(value) + ")")
    .no_rows();
}

int count_temp_table(pqxx::transaction_base &tx)
{
  return tx.query_value<int>(std::format("SELECT count(*) FROM {}", table));
}


void test_nontransaction_autocommits(pqxx::test::context &)
{
  pqxx::connection cx;

  pqxx::nontransaction tx1{cx};
  delete_temp_table(tx1);
  create_temp_table(tx1);
  tx1.commit();

  pqxx::nontransaction tx2{cx};
  insert_temp_table(tx2, 4);
  tx2.abort();

  pqxx::nontransaction tx3{cx};
  PQXX_CHECK_EQUAL(
    count_temp_table(tx3), 1,
    "Did not keep effect of aborted nontransaction.");
  delete_temp_table(tx3);
}


template<typename TX> void test_double_close(pqxx::test::context &)
{
  pqxx::connection cx;

  TX tx1{cx};
  tx1.exec("SELECT 1").one_row();
  tx1.commit();
  tx1.commit();

  TX tx2{cx};
  tx2.exec("SELECT 2").one_row();
  tx2.abort();
  tx2.abort();

  TX tx3{cx};
  tx3.exec("SELECT 3").one_row();
  tx3.commit();
  PQXX_CHECK_THROWS(tx3.abort(), pqxx::usage_error);
  ;

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
  test_double_close<pqxx::transaction<>>(tctx);
  test_double_close<pqxx::read_transaction>(tctx);
  test_double_close<pqxx::nontransaction>(tctx);
  test_double_close<pqxx::robusttransaction<>>(tctx);
  test_failed_commit<pqxx::transaction<>>(tctx);
  test_failed_commit<pqxx::robusttransaction<>>(tctx);
  test_commit_on_broken_connection<pqxx::transaction<>>(tctx);
  test_commit_on_broken_connection<pqxx::robusttransaction<>>(tctx);
}


PQXX_REGISTER_TEST(test_nontransaction_continues_after_error);
PQXX_REGISTER_TEST(test_nontransaction_autocommits);
PQXX_REGISTER_TEST(test_transaction);
} // namespace
