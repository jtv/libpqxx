#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;

// Test program for libpqxx's Large Objects interface.
namespace
{
const string Contents = "Large object test contents";


void test_051(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject obj = perform(
    [&C]()
    {
      work tx{C};
      largeobjectaccess A(tx);
      auto new_obj = largeobject(A);

      A.write(Contents);

      char Buf[200];
      const auto Size = sizeof(Buf) - 1;

      auto Offset = A.seek(0, ios::beg);
      PQXX_CHECK_EQUAL(Offset, 0, "Wrong position after seek to beginning.");

      PQXX_CHECK_EQUAL(
	size_t(A.read(Buf, Size)),
	Contents.size(),
	"Unexpected read() result.");

      PQXX_CHECK_EQUAL(
	std::string(Buf, Contents.size()),
	Contents,
	"Large object contents were mutilated.");

      // Now write contents again, this time as a C string
      PQXX_CHECK_EQUAL(
	A.seek(-int(Contents.size()), ios::end),
	0,
	"Bad position after seeking to beginning of large object.");

      using lobj_size_t = largeobjectaccess::size_type;
      A.write(Buf, lobj_size_t(Contents.size()));
      A.seek(0, ios::beg);
      PQXX_CHECK_EQUAL(
	size_t(A.read(Buf, Size)),
	Contents.size(),
	"Bad length for rewritten large object.");

      PQXX_CHECK_EQUAL(
	std::string(Buf, Contents.size()),
	Contents,
	"Rewritten large object was mangled.");

      tx.commit();
      return new_obj;
    });

  PQXX_CHECK(
	obj != largeobject{},
	"Large objects: false negative on operator!=().");

  PQXX_CHECK(
	not (obj == largeobject{}),
	"Large objects: false positive on operator==().");

  PQXX_CHECK(
	not (obj != obj),
	"Large objects: false positive on operator!=().");
  PQXX_CHECK(
	obj == obj,
	"Large objects: false negative on operator==().");

  PQXX_CHECK(
	obj <= obj,
	"Large objects: false negative on operator<=().");
  PQXX_CHECK(
	obj >= obj,
	"Large objects: false negative on operator>=().");

  PQXX_CHECK(
	not (obj < obj),
	"Large objects: false positive on operator<().");
  PQXX_CHECK(
	not (obj > obj),
	"Large objects: false positive on operator>().");

  perform(
    [&C, &obj]()
    {
      work tx{C};
      obj.remove(tx);
      tx.commit();
    });
}
} // namespace

PQXX_REGISTER_TEST_T(test_051, nontransaction)
