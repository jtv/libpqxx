#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void make_table(transaction_base &trans)
{
  trans.exec("CREATE TEMP TABLE foo (x INTEGER)");
}


void insert_row(transaction_base &trans)
{
  trans.exec("INSERT INTO foo(x) VALUES (1)");
}


int count_rows(transaction_base &trans)
{
  const result r = trans.exec("SELECT count(*) FROM foo");
  return r[0][0].as<int>();
}


void test_subtransaction_commits_if_commit_called(connection_base &conn)
{
  work trans(conn);
  make_table(trans);
  {
    subtransaction sub(trans);
    insert_row(sub);
    sub.commit();
  }
  PQXX_CHECK_EQUAL(
	count_rows(trans),
	1,
	"Work done in committed subtransaction was lost.");
}


void test_subtransaction_aborts_if_abort_called(connection_base &conn)
{
  work trans(conn);
  make_table(trans);
  {
    subtransaction sub(trans);
    insert_row(sub);
    sub.abort();
  }
  PQXX_CHECK_EQUAL(
	count_rows(trans),
	0,
	"Aborted subtransaction was not rolled back.");
}


void test_subtransaction_aborts_implicitly(connection_base &conn)
{
  work trans(conn);
  make_table(trans);
  {
    subtransaction sub(trans);
    insert_row(sub);
  }
  PQXX_CHECK_EQUAL(
	count_rows(trans),
	0,
	"Uncommitted subtransaction was not rolled back uring destruction.");
}


// A bug found in libpqxx 6.4.5: executing a prepared statement does not
// activate the transaction. (#512)
void test_subtransaction_starts_on_prepared_statement(connection_base &conn)
{
  work tx{conn};
  tx.exec0("CREATE TEMP TABLE pqxx_foo (x integer)");
  conn.prepare("pqxx_ins", "INSERT INTO pqxx_foo (x) VALUES (1)");
  subtransaction sub{tx};
  sub.exec_prepared("pqxx_ins");
  sub.abort();
  row count{tx.exec1("SELECT count(*) FROM pqxx_foo")};
  PQXX_CHECK_EQUAL(
	count[0].as<int>(),
	0,
	"Invoking prepared statement did not activate its (sub)transaction.");
}


void test_subtransaction()
{
  connection conn;
  test_subtransaction_commits_if_commit_called(conn);
  test_subtransaction_aborts_if_abort_called(conn);
  test_subtransaction_aborts_implicitly(conn);
  test_subtransaction_starts_on_prepared_statement(conn);
}


PQXX_REGISTER_TEST(test_subtransaction);
} // namespace
