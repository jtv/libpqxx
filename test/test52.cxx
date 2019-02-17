#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;

// Test program for libpqxx: write large object to test files.
namespace
{
const std::string Contents = "Large object test contents";


void test_052()
{
  connection conn;

  largeobject Obj = perform(
    [&conn]()
    {
      work tx{conn};
      auto obj = largeobject{tx};
      tx.commit();
      return obj;
    });

  perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      largeobjectaccess A{tx, Obj.id(), std::ios::out};
      A.write(Contents);
      tx.commit();
    });

  perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      Obj.to_file(tx, "pqxxlo.txt");
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


PQXX_REGISTER_TEST(test_052);
} // namespace
