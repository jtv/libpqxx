#include <iostream>

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
  std::cout << T0.exec1("SELECT 'T0 starts'")[0].c_str() << std::endl;
  pqxx::subtransaction T0a(T0, "T0a");
  T0a.commit();
  pqxx::subtransaction T0b(T0, "T0b");
  T0b.abort();
  std::cout << T0.exec1("SELECT 'T0 ends'")[0].c_str() << std::endl;
  T0.commit();

  // Basic functionality: perform query in subtransaction; abort, continue
  pqxx::work T1(C, "T1");
  std::cout << T1.exec1("SELECT 'T1 starts'")[0].c_str() << std::endl;
  pqxx::subtransaction T1a(T1, "T1a");
  std::cout << T1a.exec1("SELECT '  a'")[0].c_str() << std::endl;
  T1a.commit();
  pqxx::subtransaction T1b(T1, "T1b");
  std::cout << T1b.exec1("SELECT '  b'")[0].c_str() << std::endl;
  T1b.abort();
  pqxx::subtransaction T1c(T1, "T1c");
  std::cout << T1c.exec1("SELECT '  c'")[0].c_str() << std::endl;
  T1c.commit();
  std::cout << T1.exec1("SELECT 'T1 ends'")[0].c_str() << std::endl;
  T1.commit();
}
} // namespace


PQXX_REGISTER_TEST(test_089);
