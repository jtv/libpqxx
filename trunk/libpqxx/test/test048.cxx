#include <iostream>
#include <sstream>

#include <pqxx/all.h>
#include <pqxx/largeobject.h>

using namespace PGSTD;
using namespace pqxx;

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

class WriteLargeObject : public Transactor
{
public:
  WriteLargeObject(const string &Contents, Oid &ID) : 
    Transactor("WriteLargeObject"),
    m_Contents(Contents),
    m_Object(InvalidOid),
    m_ObjectOutput(ID)
  {
  }

  void operator()(argument_type &T)
  {
    LargeObject L(T);
    m_Object = L.id();
    cout << "Created large object #" << m_Object << endl;

    largeobject_streambuf<> O(T, L, ios::out);
    ostream OS(&O);
    OS << m_Contents << endl;
  }

  void OnCommit()
  {
    m_ObjectOutput = m_Object;
  }

private:
  string m_Contents;
  Oid m_Object;
  Oid &m_ObjectOutput;
};


class ReadLargeObject : public Transactor
{
public:
  ReadLargeObject(string &Contents, Oid ID) :
    Transactor("ReadLargeObject"),
    m_Contents(),
    m_ContentsOutput(Contents),
    m_Object(ID)
  {
  }

  void operator()(argument_type &T)
  {
    largeobject_streambuf<> I(T, m_Object, ios::in);
    istream S(&I);
    m_Contents = UnStream(S);
  }

  void OnCommit()
  {
    m_ContentsOutput = m_Contents;
  }

private:
  string m_Contents;
  string &m_ContentsOutput;
  Oid m_Object;
};

}


// Simple test program for libpqxx's Large Objects interface.
//
// Usage: test48 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    Connection C(argv[1]);

    Oid Obj;
    const string Contents = "Testing, testing, 1-2-3";

    C.Perform(WriteLargeObject(Contents, Obj));

    string Readback;		// Contents as read back from large object
    C.Perform(ReadLargeObject(Readback, Obj));

    /* Reconstruct what will happen to our contents string if we put it into a
     * stream and then read it back.  We can compare this with what comes back
     * from our large object stream.
     */
    stringstream TestStream;
    TestStream << Contents;
    const string StreamedContents = UnStream(TestStream);

    cout << StreamedContents << endl << Readback << endl;

    if (Readback != StreamedContents)
      throw logic_error("Large objects: expected to read "
	                "'" + StreamedContents + "', "
			"got '" + Readback + "'");
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


