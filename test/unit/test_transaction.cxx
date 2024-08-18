#include <pqxx/nontransaction>
#include <pqxx/robusttransaction>

#include "../test_helpers.hxx"


namespace
{
void test_nontransaction_continues_after_error()
{
  pqxx::connection cx;
  pqxx::nontransaction tx{cx};

  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 9"), 9, "Simple query went wrong.");
  PQXX_CHECK_THROWS(
    tx.exec("SELECT 1/0"), pqxx::sql_error, "Expected error did not happen.");

  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 5"), 5, "Wrong result after error.");
}


std::string const table{"pqxx_test_transaction"};


void delete_temp_table(pqxx::transaction_base &tx)
{
  tx.exec(std::string{"DROP TABLE IF EXISTS "} + table).no_rows();
}


void create_temp_table(pqxx::transaction_base &tx)
{
  tx.exec("CREATE TEMP TABLE " + table + " (x integer)").no_rows();
}


void insert_temp_table(pqxx::transaction_base &tx, int value)
{
  tx.exec(
      "INSERT INTO " + table + " (x) VALUES (" + pqxx::to_string(value) + ")")
    .no_rows();
}

int count_temp_table(pqxx::transaction_base &tx)
{
  return tx.query_value<int>("SELECT count(*) FROM " + table);
}


void test_nontransaction_autocommits()
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


template<typename TX> void test_double_close()
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
  PQXX_CHECK_THROWS(
    tx3.abort(), pqxx::usage_error, "Abort after commit not caught.");
  ;

  TX tx4{cx};
  tx4.exec("SELECT 4").one_row();
  tx4.abort();
  PQXX_CHECK_THROWS(
    tx4.commit(), pqxx::usage_error, "Commit after abort not caught.");
}


void test_transaction()
{
  test_nontransaction_continues_after_error();
  test_nontransaction_autocommits();
  test_double_close<pqxx::transaction<>>();
  test_double_close<pqxx::read_transaction>();
  test_double_close<pqxx::nontransaction>();
  test_double_close<pqxx::robusttransaction<>>();
}


PQXX_REGISTER_TEST(test_transaction);
} // namespace
