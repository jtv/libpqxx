#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx: import file to large object
namespace
{
const string Contents = "Large object test contents";


void test_055(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj = perform(
    [&C]()
    {
      char Buf[200];
      work tx{C};
      largeobjectaccess A{tx, "pqxxlo.txt", ios::in};
      auto new_obj = largeobject(A);
      const auto len = A.read(Buf, sizeof(Buf)-1);
      PQXX_CHECK_EQUAL(
	string(Buf, string::size_type(len)),
	Contents,
	"Large object contents were mangled.");

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

PQXX_REGISTER_TEST_T(test_055, nontransaction)
