#include <iostream>
#include <sstream>

#include <pqxx/all.h>
#include <pqxx/largeobject.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";


class WriteLargeObject : public Transactor
{
public:
  explicit WriteLargeObject(LargeObject &O) : 
    Transactor("WriteLargeObject"),
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    LargeObjectAccess A(T);
    cout << "Created large object #" << A.id() << endl;
    m_Object = LargeObject(A);

    A.write(Contents);

    char Buf[200];
    const LargeObjectAccess::size_type Size = sizeof(Buf) - 1;

    LargeObjectAccess::size_type Offset = A.seek(0, ios::beg);
    if (Offset != 0)
      throw logic_error("After seeking to start of large object, "
	                "seek() returned " + ToString(Offset));

    LargeObjectAccess::size_type Read = A.read(Buf, Size);
    if (Read > Size)
      throw logic_error("Tried to read " + ToString(Size) + " bytes "
	                "from large object, got " + ToString(Read));

    Buf[Read] = '\0';
    if (Contents != Buf)
      throw logic_error("Wrote '" + Contents + "' to large object, "
	                "got '" + Buf + "' back");

    // Now write contents again, this time as a C string
    Offset = A.seek(-Read, ios::end);
    if (Offset != 0)
      throw logic_error("Tried to seek back to beginning, got " +
	                ToString(Offset));

    A.write(Buf, Read);
    A.seek(0, ios::beg);
    Read = A.read(Buf, Size);
    Buf[Read] = '\0';
    if (Contents != Buf)
      throw logic_error("Rewrote '" + Contents + "' to large object, "
	                "got '" + Buf + "' back");
  }

  void OnCommit()
  {
    if (!(m_ObjectOutput != m_Object))
      throw logic_error("Large objects: false negative on !=");
    if (m_ObjectOutput == m_Object)
      throw logic_error("Large objects: false positive on ==");

    m_ObjectOutput = m_Object;

    if (m_ObjectOutput != m_Object)
      throw logic_error("Large objects: false positive on !=");
    if (!(m_ObjectOutput == m_Object))
      throw logic_error("Large objects: false negative on ==");

    if (!(m_ObjectOutput <= m_Object))
      throw logic_error("Large objects: false negative on <=");
    if (!(m_ObjectOutput >= m_Object))
      throw logic_error("Large objects: false negative on >=");
    if (m_ObjectOutput < m_Object)
      throw logic_error("Large objects: false positive on <");
    if (m_ObjectOutput > m_Object)
      throw logic_error("Large objects: false positive on >");
 }

private:
  LargeObject m_Object;
  LargeObject &m_ObjectOutput;
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


