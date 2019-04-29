#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Attempt to perform nested queries on various types
// of connections.
namespace
{
void do_test(connection_base &C, const std::string &desc)
{
  std::cout << "Testing " << desc << ":" << std::endl;

  // Trivial test: create subtransactions, and commit/abort
  work T0(C, "T0");
  std::cout << T0.exec1("SELECT 'T0 starts'")[0].c_str() << std::endl;
  subtransaction T0a(T0, "T0a");
  T0a.commit();
  subtransaction T0b(T0, "T0b");
  T0b.abort();
  std::cout << T0.exec1("SELECT 'T0 ends'")[0].c_str() << std::endl;
  T0.commit();

  // Basic functionality: perform query in subtransaction; abort, continue
  work T1(C, "T1");
  std::cout << T1.exec1("SELECT 'T1 starts'")[0].c_str() << std::endl;
  subtransaction T1a(T1, "T1a");
    std::cout << T1a.exec1("SELECT '  a'")[0].c_str() << std::endl;
    T1a.commit();
  subtransaction T1b(T1, "T1b");
    std::cout << T1b.exec1("SELECT '  b'")[0].c_str() << std::endl;
    T1b.abort();
  subtransaction T1c(T1, "T1c");
    std::cout << T1c.exec1("SELECT '  c'")[0].c_str() << std::endl;
    T1c.commit();
  std::cout << T1.exec1("SELECT 'T1 ends'")[0].c_str() << std::endl;
  T1.commit();
}


void test_089()
{
  connection C1;
  do_test(C1, "connection");

  connection C2;
  C2.activate();
  C2.deactivate();
  C2.activate();
  do_test(C2, "reactivated connection");

  asyncconnection A;
  A.activate();
  do_test(A, "asyncconnection");

  lazyconnection L;
  L.activate();
  do_test(L, "lazyconnection");
}
} // namespace


PQXX_REGISTER_TEST(test_089);
