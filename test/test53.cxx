#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx: import file to large object
namespace
{
const std::string Contents = "Large object test contents";


void test_053()
{
  connection conn;

  largeobject Obj = perform(
    [&conn]()
    {
      work tx{conn};
      auto obj = largeobject{tx, "pqxxlo.txt"};
      tx.commit();
      return obj;
    });

  perform(
    [&conn, &Obj]()
    {
      char Buf[200];
      work tx{conn};
      largeobjectaccess O{tx, Obj, std::ios::in};
      const auto len = O.read(Buf, sizeof(Buf)-1);
      PQXX_CHECK_EQUAL(
	std::string(Buf, std::string::size_type(len)),
	Contents,
	"Large object contents were mangled.");
      tx.commit();
    });

  perform([&conn, &Obj](){ work tx{conn}; Obj.remove(tx); tx.commit(); });
}


PQXX_REGISTER_TEST(test_053);
} // namespace
