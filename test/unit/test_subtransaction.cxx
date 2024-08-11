#include <pqxx/subtransaction>
#include <pqxx/transaction>

#include "../test_helpers.hxx"

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


void test_subtransaction_commits_if_commit_called(pqxx::connection &cx)
{
  pqxx::work tx(cx);
  make_table(tx);
  {
    pqxx::subtransaction sub(tx);
    insert_row(sub);
    sub.commit();
  }
  PQXX_CHECK_EQUAL(
    count_rows(tx), 1, "Work done in committed subtransaction was lost.");
}


void test_subtransaction_aborts_if_abort_called(pqxx::connection &cx)
{
  pqxx::work tx(cx);
  make_table(tx);
  {
    pqxx::subtransaction sub(tx);
    insert_row(sub);
    sub.abort();
  }
  PQXX_CHECK_EQUAL(
    count_rows(tx), 0, "Aborted subtransaction was not rolled back.");
}


void test_subtransaction_aborts_implicitly(pqxx::connection &cx)
{
  pqxx::work tx(cx);
  make_table(tx);
  {
    pqxx::subtransaction sub(tx);
    insert_row(sub);
  }
  PQXX_CHECK_EQUAL(
    count_rows(tx), 0,
    "Uncommitted subtransaction was not rolled back uring destruction.");
}


void test_subtransaction()
{
  pqxx::connection cx;
  test_subtransaction_commits_if_commit_called(cx);
  test_subtransaction_aborts_if_abort_called(cx);
  test_subtransaction_aborts_implicitly(cx);
}


PQXX_REGISTER_TEST(test_subtransaction);
} // namespace
