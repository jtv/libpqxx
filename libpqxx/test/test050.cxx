#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";

class CreateLargeObject : public transactor<>
{
public:
  explicit CreateLargeObject(largeobject &O) :
    transactor<>("CreateLargeObject"),
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    m_Object = largeobject(T);
    cout << "Created large object #" << m_Object.id() << endl;
  }

  void OnCommit()
  {
    m_ObjectOutput = m_Object;
  }

private:
  largeobject m_Object;
  largeobject &m_ObjectOutput;
};

class WriteLargeObject : public transactor<>
{
public:
  explicit WriteLargeObject(largeobject &O) : 
    transactor<>("WriteLargeObject"),
    m_Object(O)
  {
  }

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T, m_Object);
    cout << "Writing to large object #" << largeobject(A).id() << endl;
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
  largeobject m_Object;
};


class DeleteLargeObject : public transactor<>
{
public:
  explicit DeleteLargeObject(largeobject O) : m_Object(O) {}

  void operator()(argument_type &T)
  {
    m_Object.remove(T);
  }

private:
  largeobject m_Object;
};


}


// Simple test program for libpqxx's Large Objects interface.
//
// Usage: test050 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);

    largeobject Obj;

    C.perform(CreateLargeObject(Obj));
    C.perform(WriteLargeObject(Obj));
    C.perform(DeleteLargeObject(Obj));
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


