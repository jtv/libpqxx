#include <pqxx/subtransaction>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void make_table(pqxx::transaction_base &tx)
{
  tx.exec("CREATE TEMP TABLE foo (x INTEGER)").no_rows();
}


void insert_row(pqxx::transaction_base &tx)
{
  tx.exec("INSERT INTO foo(x) VALUES (1)").no_rows();
}


int count_rows(pqxx::transaction_base &tx)
{
  return tx.query_value<int>("SELECT count(*) FROM foo");
}


void test_subtransaction_commits_if_commit_called(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx(cx);
  make_table(tx);
  {
    pqxx::subtransaction sub(tx);
    insert_row(sub);
    sub.commit();
  }
  // Work done in a committed subtransaction is preserved while the main
  // transaction remains active.
  PQXX_CHECK_EQUAL(count_rows(tx), 1);

  // After that, we can continue as normal.
  insert_row(tx);
  PQXX_CHECK_EQUAL(count_rows(tx), 2);
}


void test_subtransaction_aborts_if_abort_called(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx(cx);
  make_table(tx);
  {
    pqxx::subtransaction sub(tx);
    insert_row(sub);
    sub.abort();
  }
  // Work done in an aborted subtranction is immediately rolled back.
  PQXX_CHECK_EQUAL(count_rows(tx), 0);

  // After that, we can continue as normal.
  insert_row(tx);
  PQXX_CHECK_EQUAL(count_rows(tx), 1);
}


void test_subtransaction_aborts_implicitly(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx(cx);
  make_table(tx);
  {
    pqxx::subtransaction sub(tx);
    insert_row(sub);
  }
  // Destructor implicitly rolls back an uncommitted subtransaction.
  PQXX_CHECK_EQUAL(count_rows(tx), 0);

  // After that, we can continue as normal.
  insert_row(tx);
  PQXX_CHECK_EQUAL(count_rows(tx), 1);
}


PQXX_REGISTER_TEST(test_subtransaction_commits_if_commit_called);
PQXX_REGISTER_TEST(test_subtransaction_aborts_if_abort_called);
PQXX_REGISTER_TEST(test_subtransaction_aborts_implicitly);
} // namespace
