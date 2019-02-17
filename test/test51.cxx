#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;

// Test program for libpqxx's Large Objects interface.
namespace
{
const std::string Contents = "Large object test contents";


void test_051()
{
  connection conn;

  largeobject obj = perform(
    [&conn]()
    {
      work tx{conn};
      largeobjectaccess A(tx);
      auto new_obj = largeobject(A);

      A.write(Contents);

      char Buf[200];
      const auto Size = sizeof(Buf) - 1;

      auto Offset = A.seek(0, std::ios::beg);
      PQXX_CHECK_EQUAL(Offset, 0, "Wrong position after seek to beginning.");

      PQXX_CHECK_EQUAL(
	size_t(A.read(Buf, Size)),
	Contents.size(),
	"Unexpected read() result.");

      PQXX_CHECK_EQUAL(
	std::string(Buf, Contents.size()),
	Contents,
	"Large object contents were mutilated.");

      // Now write contents again, this time as a conn string
      PQXX_CHECK_EQUAL(
	A.seek(-int(Contents.size()), std::ios::end),
	0,
	"Bad position after seeking to beginning of large object.");

      using lobj_size_t = largeobjectaccess::size_type;
      A.write(Buf, lobj_size_t(Contents.size()));
      A.seek(0, std::ios::beg);
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
    [&conn, &obj]()
    {
      work tx{conn};
      obj.remove(tx);
      tx.commit();
    });
}


PQXX_REGISTER_TEST(test_051);
} // namespace
