#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx: write large object to test files.
namespace
{
const string Contents = "Large object test contents";


void test_054(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj = perform(
    [&C]()
    {
      work tx{C};
      largeobjectaccess A(tx);
      auto new_obj = largeobject(A);
      A.write(Contents);
      A.to_file("pqxxlo.txt");
      tx.commit();
      return new_obj;
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

PQXX_REGISTER_TEST_T(test_054, nontransaction)
