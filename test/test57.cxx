#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx's Large Objects interface.
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


void test_057(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  const string Contents = "Testing, testing, 1-2-3";

  largeobject Obj = perform(
    [&C, &Contents]()
    {
      work tx{C};
      auto new_obj = largeobject{tx};
      olostream S(tx, new_obj.id());
      S << Contents;
      S.flush();
      tx.commit();
      return new_obj;
    });

  const string Readback = perform(
    [&C, &Obj]()
    {
      work tx{C};
      ilostream S(tx, Obj);
      return UnStream(S);
    });

  perform(
    [&C, &Obj]()
    {
      work tx{C};
      Obj.remove(tx);
      tx.commit();
    });

  /* Reconstruct what will happen to our contents string if we put it into a
   * stream and then read it back.  We can compare this with what comes back
   * from our large object stream.
   */
  stringstream TestStream;
  TestStream << Contents;
  const string StreamedContents = UnStream(TestStream);

  cout << StreamedContents << endl << Readback << endl;

  PQXX_CHECK_EQUAL(Readback, StreamedContents, "Contents were mangled.");
}
} // namespace

PQXX_REGISTER_TEST_T(test_057, nontransaction)
