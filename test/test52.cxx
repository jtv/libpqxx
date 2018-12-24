#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;

// Test program for libpqxx: write large object to test files.
namespace
{
const string Contents = "Large object test contents";


void test_052(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj = perform(
    [&C]()
    {
      work tx{C};
      auto obj = largeobject{tx};
      tx.commit();
      return obj;
    });

  perform(
    [&C, &Obj]()
    {
      work tx{C};
      largeobjectaccess A{tx, Obj.id(), ios::out};
      A.write(Contents);
      tx.commit();
    });

  perform(
    [&C, &Obj]()
    {
      work tx{C};
      Obj.to_file(tx, "pqxxlo.txt");
      tx.commit();
    });

  perform(
    [&C, &Obj]()
    {
      work tx{C};
      Obj.remove(tx);
      tx.commit();
    });
}

} // namespace

PQXX_REGISTER_TEST_T(test_052, nontransaction)
