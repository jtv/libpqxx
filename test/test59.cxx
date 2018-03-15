#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx's Large Objects interface.
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


class WriteLargeObject : public transactor<>
{
public:
  WriteLargeObject(const string &Contents, largeobject &O) :
    transactor<>("WriteLargeObject"),
    m_contents(Contents),
    m_object(),
    m_object_output(O)
  {
  }

  void operator()(argument_type &T)
  {
    m_object = largeobject(T);
    cout << "Created large object #" << m_object.id() << endl;

    lostream S(T, m_object.id());
    S << m_contents;
  }

  void on_commit()
  {
    m_object_output = m_object;
  }

private:
  string m_contents;
  largeobject m_object;
  largeobject &m_object_output;
};


class ReadLargeObject : public transactor<>
{
public:
  ReadLargeObject(string &Contents, largeobject O) :
    transactor<>("ReadLargeObject"),
    m_contents(),
    m_contents_output(Contents),
    m_object(O)
  {
  }

  void operator()(argument_type &T)
  {
    lostream S(T, m_object, ios::in);
    m_contents = UnStream(S);
  }

  void on_commit()
  {
    m_contents_output = m_contents;
  }

private:
  string m_contents;
  string &m_contents_output;
  largeobject m_object;
};


class DeleteLargeObject : public transactor<>
{
public:
  explicit DeleteLargeObject(largeobject O) : m_object(O) {}

  void operator()(argument_type &T)
  {
    m_object.remove(T);
  }

private:
  largeobject m_object;
};


void test_059(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj(oid_none);
  const string Contents = "Testing, testing, 1-2-3";

  C.perform(WriteLargeObject(Contents, Obj));

  string Readback;		// Contents as read back from large object
  C.perform(ReadLargeObject(Readback, Obj));

  C.perform(DeleteLargeObject(Obj));

  /* Reconstruct what will happen to our contents string if we put it into a
   * stream and then read it back.  We can compare this with what comes back
   * from our large object stream.
   */
  stringstream TestStream;
  TestStream << Contents;
  const string StreamedContents = UnStream(TestStream);

  cout << StreamedContents << endl << Readback << endl;

  PQXX_CHECK_EQUAL(
	Readback,
	StreamedContents,
	"Large object contents were mangled.");
}
} // namespace

PQXX_REGISTER_TEST(test_059)
