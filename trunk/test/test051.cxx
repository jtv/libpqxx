#include <iostream>
#include <sstream>

#include <pqxx/pqxx>


using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";


class WriteLargeObject : public transactor<>
{
public:
  explicit WriteLargeObject(largeobject &O) : 
    transactor<>("WriteLargeObject"),
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T);
    cout << "Created large object #" << A.id() << endl;
    m_Object = largeobject(A);

    A.write(Contents);

    char Buf[200];
    const largeobjectaccess::size_type Size = sizeof(Buf) - 1;

    largeobjectaccess::size_type Offset = A.seek(0, ios::beg);
    if (Offset != 0)
      throw logic_error("After seeking to start of large object, "
	                "seek() returned " + to_string(Offset));

    largeobjectaccess::size_type Read = A.read(Buf, Size);
    if (Read > Size)
      throw logic_error("Tried to read " + to_string(Size) + " bytes "
	                "from large object, got " + to_string(Read));

    Buf[Read] = '\0';
    if (Contents != Buf)
      throw logic_error("Wrote '" + Contents + "' to large object, "
	                "got '" + Buf + "' back");

    // Now write contents again, this time as a C string
    Offset = A.seek(-Read, ios::end);
    if (Offset != 0)
      throw logic_error("Tried to seek back to beginning, got " +
	                to_string(Offset));

    A.write(Buf, Read);
    A.seek(0, ios::beg);
    Read = A.read(Buf, Size);
    Buf[Read] = '\0';
    if (Contents != Buf)
      throw logic_error("Rewrote '" + Contents + "' to large object, "
	                "got '" + Buf + "' back");
  }

  void on_commit()
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
  largeobject m_Object;
  largeobject &m_ObjectOutput;
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
// Usage: test051 [connect-string]
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


