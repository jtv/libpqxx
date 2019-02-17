#include <cerrno>
#include <cstring>
#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;

namespace
{
const std::string Contents = "Large object test contents";


// Mixed-mode, seeking test program for libpqxx's Large Objects interface.
void test_058()
{
  connection conn;

  perform(
    [&conn]()
    {
      work tx{conn};
      largeobjectaccess A(tx);
      A.write(Contents);

      char Buf[200];
      const size_t Size = sizeof(Buf) - 1;
      PQXX_CHECK_EQUAL(
	A.read(Buf, Size),
	0,
	"Could read bytes from large object after writing.");

      // Overwrite terminating zero.
      auto Here = A.seek(-1, std::ios::cur);
      PQXX_CHECK_EQUAL(
	Here,
	largeobject::size_type(Contents.size()-1),
	"Ended up in wrong place after moving back 1 byte.");

      A.write("!", 1);

      // Now check that we really did.
      PQXX_CHECK_EQUAL(
	A.seek(-1, std::ios::cur),
	largeobject::size_type(Contents.size()-1),
	"Inconsistent seek.");

      char Check;
      PQXX_CHECK_EQUAL(
	A.read(&Check, 1),
	1,
	"Unexpected result from read().");
      PQXX_CHECK_EQUAL(
	std::string(&Check, 1),
	"!",
	"Read back wrong character.");

      PQXX_CHECK_EQUAL(
	A.seek(0, std::ios::beg),
	0,
	"Ended up in wrong place after seeking back to beginning.");

      PQXX_CHECK_EQUAL(
	A.read(&Check, 1),
	1,
	"Unexpected result when trying to read back 1st byte.");

      PQXX_CHECK_EQUAL(
	std::string(&Check, 1),
	std::string(Contents, 0, 1),
	"Wrong first character in large object.");

      // Clean up after ourselves
      A.remove(tx);
      tx.commit();
    });
}


PQXX_REGISTER_TEST(test_058);
} // namespace
