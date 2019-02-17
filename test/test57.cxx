#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx's Large Objects interface.
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


void test_057()
{
  connection conn;

  const std::string Contents = "Testing, testing, 1-2-3";

  largeobject Obj = perform(
    [&conn, &Contents]()
    {
      work tx{conn};
      auto new_obj = largeobject{tx};
      olostream S(tx, new_obj.id());
      S << Contents;
      S.flush();
      tx.commit();
      return new_obj;
    });

  const std::string Readback = perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      ilostream S(tx, Obj);
      return UnStream(S);
    });

  perform(
    [&conn, &Obj]()
    {
      work tx{conn};
      Obj.remove(tx);
      tx.commit();
    });

  /* Reconstruct what will happen to our contents string if we put it into a
   * stream and then read it back.  We can compare this with what comes back
   * from our large object stream.
   */
  std::stringstream TestStream;
  TestStream << Contents;
  const std::string StreamedContents = UnStream(TestStream);

  std::cout << StreamedContents << std::endl << Readback << std::endl;

  PQXX_CHECK_EQUAL(Readback, StreamedContents, "Contents were mangled.");
}


PQXX_REGISTER_TEST(test_057);
} // namespace
