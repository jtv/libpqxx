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
  pqxx::connection C;

  // Trivial test: create subtransactions, and commit/abort
  pqxx::work T0(C, "T0");
  T0.exec("SELECT 'T0 starts'").one_row();
  pqxx::subtransaction T0a(T0, "T0a");
  T0a.commit();
  pqxx::subtransaction T0b(T0, "T0b");
  T0b.abort();
  T0.exec("SELECT 'T0 ends'").one_row();
  T0.commit();

  // Basic functionality: perform query in subtransaction; abort, continue
  pqxx::work T1(C, "T1");
  T1.exec("SELECT 'T1 starts'").one_row();
  pqxx::subtransaction T1a(T1, "T1a");
  T1a.exec("SELECT '  a'").one_row();
  T1a.commit();
  pqxx::subtransaction T1b(T1, "T1b");
  T1b.exec("SELECT '  b'").one_row();
  T1b.abort();
  pqxx::subtransaction T1c(T1, "T1c");
  T1c.exec("SELECT '  c'").one_row();
  T1c.commit();
  T1.exec("SELECT 'T1 ends'").one_row();
  T1.commit();
}
} // namespace


PQXX_REGISTER_TEST(test_089);
