#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

#include <pqxx/all.h>
#include <pqxx/largeobject.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";

class CreateLargeObject : public Transactor
{
public:
  explicit CreateLargeObject(LargeObject &O) :
    Transactor("CreateLargeObject"),
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    m_Object = LargeObject(T);
    cout << "Created large object #" << m_Object.id() << endl;
  }

  void OnCommit()
  {
    m_ObjectOutput = m_Object;
  }

private:
  LargeObject m_Object;
  LargeObject &m_ObjectOutput;
};

class WriteLargeObject : public Transactor
{
public:
  explicit WriteLargeObject(LargeObject &O) : 
    Transactor("WriteLargeObject"),
    m_Object(O)
  {
  }

  void operator()(argument_type &T)
  {
    LargeObjectAccess A(T, m_Object);
    cout << "Writing to large object #" << LargeObject(A).id() << endl;
    long Bytes = A.cwrite(Contents.c_str(), Contents.size());
    if (Bytes != long(Contents.size()))
      throw logic_error("Tried to write " + ToString(Contents.size()) + " "
	                "bytes to large object, but wrote " + ToString(Bytes));

    char Buf[200];
    const size_t Size = sizeof(Buf) - 1;
    Bytes = A.cread(Buf, Size);
    if (Bytes)
    {
      if (Bytes < 0)
	throw runtime_error("Read error at end of large object: " +
	                    string(strerror(errno)));
      throw logic_error("Could read " + ToString(Bytes) + " bytes "
	                "from large object after writing");
    }

    string::size_type Offset = A.cseek(0, ios::cur);
    if (Offset != Contents.size())
      throw logic_error("Expected to be at position " + 
	                 ToString(Contents.size()) + " in large object, "
			 "but cseek(0, cur) returned " + ToString(Offset));
    
    Offset = A.cseek(1, ios::beg);
    if (Offset != 1)
      throw logic_error("After seeking to position 1 in large object, cseek() "
	                "returned " + ToString(Offset));

    Offset = A.cseek(-1, ios::cur);
    if (Offset != 0)
      throw logic_error("After seeking -1 from position 1 in large object, "
	                "cseek() returned " + ToString(Offset));

    const size_t Read = A.read(Buf, Size);
    if (Read > Size)
      throw logic_error("Tried to read " + ToString(Size) + " bytes "
	                "from large object, got " + ToString(Read));

    Buf[Read] = '\0';
    if (Contents != Buf)
      throw logic_error("Wrote '" + Contents + "' to large object, "
	                "got '" + Buf + "' back");
  }

private:
  LargeObject m_Object;
};


class DeleteLargeObject : public Transactor
{
public:
  explicit DeleteLargeObject(LargeObject O) : m_Object(O) {}

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
// Usage: test50 [connect-string]
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

    LargeObject Obj;

    C.Perform(CreateLargeObject(Obj));
    C.Perform(WriteLargeObject(Obj));
    C.Perform(DeleteLargeObject(Obj));
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


