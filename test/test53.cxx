#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx: import file to large object
namespace
{
const string Contents = "Large object test contents";


void test_053(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj = perform(
    [&C]()
    {
      work tx{C};
      auto obj = largeobject{tx, "pqxxlo.txt"};
      tx.commit();
      return obj;
    });

  perform(
    [&C, &Obj]()
    {
      char Buf[200];
      work tx{C};
      largeobjectaccess O{tx, Obj, ios::in};
      const auto len = O.read(Buf, sizeof(Buf)-1);
      PQXX_CHECK_EQUAL(
	string(Buf, string::size_type(len)),
	Contents,
	"Large object contents were mangled.");
      tx.commit();
    });

  perform([&C, &Obj](){ work tx{C}; Obj.remove(tx); tx.commit(); });
}

} // namespace

PQXX_REGISTER_TEST_T(test_053, nontransaction)
