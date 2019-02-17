#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx's Large Objects interface.
namespace
{
const std::string Contents = "Large object test contents";


void test_050()
{
  connection conn;

  // Create a large object.
  largeobject Obj = perform(
    [&conn]()
    {
      work tx{conn};
      const auto obj = largeobject(tx);
      tx.commit();
      return obj;
    });

  // Write to the large object, and play with it a little.
  perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      largeobjectaccess A(tx, Obj);

      const auto
	orgpos = A.ctell(),
	copyorgpos = A.ctell();

      PQXX_CHECK_EQUAL(orgpos, 0, "Bad initial position in large object.");
      PQXX_CHECK_EQUAL(copyorgpos, orgpos, "ctell() affected positioning.");

      const largeobjectaccess::pos_type cxxorgpos = A.tell();
      PQXX_CHECK_EQUAL(cxxorgpos, orgpos, "tell() reports bad position.");

      A.process_notice("Writing to large object #" +
	to_string(largeobject(A).id()) + "\n");
      long Bytes = A.cwrite(
	Contents.c_str(),
	largeobject::size_type(Contents.size()));

      PQXX_CHECK_EQUAL(
	Bytes,
	long(Contents.size()),
	"Wrote wrong number of bytes.");

      PQXX_CHECK_EQUAL(
	A.tell(),
	A.ctell(),
	"tell() is inconsistent with ctell().");

      PQXX_CHECK_EQUAL(A.tell(), Bytes, "Bad large-object position.");

      char Buf[200];
      const size_t Size = sizeof(Buf) - 1;
      PQXX_CHECK_EQUAL(
	A.cread(Buf, Size),
	0,
	"Bad return value from cread() after writing.");

      PQXX_CHECK_EQUAL(
	size_t(A.cseek(0, std::ios::cur)),
	Contents.size(),
	"Unexpected position after cseek(0, cur).");

      PQXX_CHECK_EQUAL(
	A.cseek(1, std::ios::beg),
	1,
	"Unexpected cseek() result after seeking to position 1.");

      PQXX_CHECK_EQUAL(
	A.cseek(-1, std::ios::cur),
	0,
	"Unexpected cseek() result after seeking -1 from position 1.");

      PQXX_CHECK(size_t(A.read(Buf, Size)) <= Size, "Got too many bytes.");

      PQXX_CHECK_EQUAL(
	std::string(Buf, std::string::size_type(Bytes)),
	Contents,
	"Large-object contents were mutilated.");

      tx.commit();
    });

  perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      Obj.remove(tx);
      tx.commit();
    });
}


PQXX_REGISTER_TEST(test_050);
} // namespace
