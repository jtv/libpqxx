#include <iostream>
#include <sstream>
#include <string>

#include <pqxx/pqxx>

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

class WriteLargeObject : public transactor<>
{
public:
  WriteLargeObject(const string &Contents, largeobject &O) :
    transactor<>("WriteLargeObject"),
    m_Contents(Contents),
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    m_Object = largeobject(T);
    cout << "Created large object #" << m_Object.id() << endl;

    lostream S(T, m_Object.id());
    S << m_Contents;
  }

  void on_commit()
  {
    m_ObjectOutput = m_Object;
  }

private:
  string m_Contents;
  largeobject m_Object;
  largeobject &m_ObjectOutput;
};


class ReadLargeObject : public transactor<>
{
public:
  ReadLargeObject(string &Contents, largeobject O) :
    transactor<>("ReadLargeObject"),
    m_Contents(),
    m_ContentsOutput(Contents),
    m_Object(O)
  {
  }

  void operator()(argument_type &T)
  {
    lostream S(T, m_Object, ios::in);
    m_Contents = UnStream(S);
  }

  void on_commit()
  {
    m_ContentsOutput = m_Contents;
  }

private:
  string m_Contents;
  string &m_ContentsOutput;
  largeobject m_Object;
};


class DeleteLargeObject : public transactor<>
{
public:
  DeleteLargeObject(largeobject O) : m_Object(O) {}

  void operator()(argument_type &T)
  {
    m_Object.remove(T);
  }

private:
  largeobject m_Object;
};


}


// Simple test program for libpqxx's large objects on asynchronous connection.
//
// Usage: test065 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    const string connstr = (argv[1] ? argv[1] : "");
    asyncconnection C(connstr);

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

    if (Readback != StreamedContents)
      throw logic_error("Large objects: expected to read "
	                "'" + StreamedContents + "', "
			"got '" + Readback + "'");
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
    return 1;
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


