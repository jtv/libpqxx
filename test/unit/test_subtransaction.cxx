#include "../test_helpers.hxx"

namespace
{
void make_table(pqxx::transaction_base &trans)
{
  trans.exec("CREATE TEMP TABLE foo (x INTEGER)");
}


void insert_row(pqxx::transaction_base &trans)
{
  trans.exec("INSERT INTO foo(x) VALUES (1)");
}


int count_rows(pqxx::transaction_base &trans)
{
  pqxx::result const r{trans.exec("SELECT count(*) FROM foo")};
  return r[0][0].as<int>();
}


void test_subtransaction_commits_if_commit_called(pqxx::connection_base &conn)
{
  pqxx::work trans(conn);
  make_table(trans);
  {
    pqxx::subtransaction sub(trans);
    insert_row(sub);
    sub.commit();
  }
  PQXX_CHECK_EQUAL(
    count_rows(trans), 1, "Work done in committed subtransaction was lost.");
}


void test_subtransaction_aborts_if_abort_called(pqxx::connection_base &conn)
{
  pqxx::work trans(conn);
  make_table(trans);
  {
    pqxx::subtransaction sub(trans);
    insert_row(sub);
    sub.abort();
  }
  PQXX_CHECK_EQUAL(
    count_rows(trans), 0, "Aborted subtransaction was not rolled back.");
}


void test_subtransaction_aborts_implicitly(pqxx::connection_base &conn)
{
  pqxx::work trans(conn);
  make_table(trans);
  {
    pqxx::subtransaction sub(trans);
    insert_row(sub);
  }
  PQXX_CHECK_EQUAL(
    count_rows(trans), 0,
    "Uncommitted subtransaction was not rolled back uring destruction.");
}


void test_subtransaction()
{
  pqxx::connection conn;
  test_subtransaction_commits_if_commit_called(conn);
  test_subtransaction_aborts_if_abort_called(conn);
  test_subtransaction_aborts_implicitly(conn);
}


PQXX_REGISTER_TEST(test_subtransaction);
} // namespace
