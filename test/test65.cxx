#include <iostream>
#include <sstream>
#include <string>

#include "test_helpers.hxx"


using namespace pqxx;


// Simple test program for libpqxx's large objects on asynchronous connection.
namespace
{
/* Read contents of Stream into a single string.  The data will go through
 * input formatting, so whitespace will be taken as separators between chunks
 * of data.
 */
template<typename T> std::string UnStream(T &Stream)
{
  std::string Result, X;
  while (Stream >> X) Result += X;
  return Result;
}


void test_065()
{
  asyncconnection conn("");

  const std::string Contents = "Testing, testing, 1-2-3";

  largeobject Obj = perform(
    [&conn, &Contents]()
    {
      work tx{conn};
      auto new_obj = largeobject{tx};
      lostream S{tx, new_obj.id()};
      S << Contents;
      S.flush();
      tx.commit();
      return new_obj;
    });

  const std::string Readback = perform(
    [&conn, &Obj]
    {
      work tx{conn};
      lostream S{tx, Obj, std::ios::in};
      return UnStream(S);
    });

  perform([&conn, &Obj](){ work tx{conn}; Obj.remove(tx); tx.commit(); });

  /* Reconstruct what will happen to our contents string if we put it into a
   * stream and then read it back.  We can compare this with what comes back
   * from our large object stream.
   */
  std::stringstream TestStream;
  TestStream << Contents;
  const std::string StreamedContents = UnStream(TestStream);

  std::cout << StreamedContents << std::endl << Readback << std::endl;

  PQXX_CHECK_EQUAL(Readback, StreamedContents, "Large object was mangled.");
}


PQXX_REGISTER_TEST(test_065);
} // namespace
