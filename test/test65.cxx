#include <iostream>
#include <sstream>
#include <string>

#include "test_helpers.hxx"


using namespace std;
using namespace pqxx;


// Simple test program for libpqxx's large objects on asynchronous connection.
namespace
{
/* Read contents of Stream into a single string.  The data will go through
 * input formatting, so whitespace will be taken as separators between chunks
 * of data.
 */
template<typename T> string UnStream(T &Stream)
{
  string Result, X;
  while (Stream >> X) Result += X;
  return Result;
}


void test_065()
{
  asyncconnection conn("");

  const string Contents = "Testing, testing, 1-2-3";

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

  const string Readback = perform(
    [&conn, &Obj]
    {
      work tx{conn};
      lostream S{tx, Obj, ios::in};
      return UnStream(S);
    });

  perform([&conn, &Obj](){ work tx{conn}; Obj.remove(tx); tx.commit(); });

  /* Reconstruct what will happen to our contents string if we put it into a
   * stream and then read it back.  We can compare this with what comes back
   * from our large object stream.
   */
  stringstream TestStream;
  TestStream << Contents;
  const string StreamedContents = UnStream(TestStream);

  cout << StreamedContents << endl << Readback << endl;

  PQXX_CHECK_EQUAL(Readback, StreamedContents, "Large object was mangled.");
}


PQXX_REGISTER_TEST(test_065);
} // namespace
