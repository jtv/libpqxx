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
  WriteLargeObject(const string &Contents, LargeObject &O) : 
    Transactor("WriteLargeObject"),
    m_Contents(Contents),
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    m_Object = LargeObject(T);
    cout << "Created large object #" << m_Object.id() << endl;

    olostream S(T, m_Object);
    S << m_Contents;
  }

  void OnCommit()
  {
    m_ObjectOutput = m_Object;
  }

private:
  string m_Contents;
  LargeObject m_Object;
  LargeObject &m_ObjectOutput;
};


class ReadLargeObject : public Transactor
{
public:
  ReadLargeObject(string &Contents, LargeObject O) :
    Transactor("ReadLargeObject"),
    m_Contents(),
    m_ContentsOutput(Contents),
    m_Object(O)
  {
  }

  void operator()(argument_type &T)
  {
    ilostream S(T, m_Object.id());
    m_Contents = UnStream(S);
  }

  void OnCommit()
  {
    m_ContentsOutput = m_Contents;
  }

private:
  string m_Contents;
  string &m_ContentsOutput;
  LargeObject m_Object;
};


class DeleteLargeObject : public Transactor
{
public:
  DeleteLargeObject(LargeObject O) : m_Object(O) {}

  void operator()(argument_type &T)
  {
    m_Object.remove(T);
  }

private:
  LargeObject m_Object;
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

    LargeObject Obj(InvalidOid);
    const string Contents = "Testing, testing, 1-2-3";

    C.Perform(WriteLargeObject(Contents, Obj));

    string Readback;		// Contents as read back from large object
    C.Perform(ReadLargeObject(Readback, Obj));

    C.Perform(DeleteLargeObject(Obj));

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


