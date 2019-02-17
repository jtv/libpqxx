#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx: write large object to test files.
namespace
{
const std::string Contents = "Large object test contents";


void test_054()
{
  connection conn;

  largeobject Obj = perform(
    [&conn]()
    {
      work tx{conn};
      largeobjectaccess A(tx);
      auto new_obj = largeobject(A);
      A.write(Contents);
      A.to_file("pqxxlo.txt");
      tx.commit();
      return new_obj;
    });

  perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      Obj.remove(tx);
      tx.commit();
    });
}


PQXX_REGISTER_TEST(test_054);
} // namespace
