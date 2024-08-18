#include <iostream>

#include <pqxx/subtransaction>
#include <pqxx/transaction>

#include "test_helpers.hxx"

// Test program for libpqxx.  Attempt to perform nested queries on various
// types of connections.
namespace
{
void test_089()
{
  pqxx::connection cx;

  // Trivial test: create subtransactions, and commit/abort
  pqxx::work tx0(cx, "tx0");
  tx0.exec("SELECT 'tx0 starts'").one_row();
  pqxx::subtransaction tx0a(tx0, "tx0a");
  tx0a.commit();
  pqxx::subtransaction tx0b(tx0, "tx0b");
  tx0b.abort();
  tx0.exec("SELECT 'tx0 ends'").one_row();
  tx0.commit();

  // Basic functionality: perform query in subtransaction; abort, continue
  pqxx::work tx1(cx, "tx1");
  tx1.exec("SELECT 'tx1 starts'").one_row();
  pqxx::subtransaction tx1a(tx1, "tx1a");
  tx1a.exec("SELECT '  a'").one_row();
  tx1a.commit();
  pqxx::subtransaction tx1b(tx1, "tx1b");
  tx1b.exec("SELECT '  b'").one_row();
  tx1b.abort();
  pqxx::subtransaction tx1c(tx1, "tx1c");
  tx1c.exec("SELECT '  c'").one_row();
  tx1c.commit();
  tx1.exec("SELECT 'tx1 ends'").one_row();
  tx1.commit();
}
} // namespace


PQXX_REGISTER_TEST(test_089);
