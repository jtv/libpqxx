#include <iostream>
#include <sstream>

#include <pqxx/all.h>
#include <pqxx/largeobject.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";

class ImportLargeObject : public Transactor
{
public:
  explicit ImportLargeObject(LargeObject &O, const string &File) :
    Transactor("ImportLargeObject"),
    m_Object(O),
    m_File(File)
  {
  }

  void operator()(argument_type &T)
  {
    m_Object = LargeObject(T, m_File);
    cout << "Imported '" << m_File << "' "
            "to large object #" << m_Object.id() << endl;
  }

private:
  LargeObject &m_Object;
  string m_File;
};

class ReadLargeObject : public Transactor
{
public:
  explicit ReadLargeObject(LargeObject &O) : 
    Transactor("ReadLargeObject"),
    m_Object(O)
  {
  }

  void operator()(argument_type &T)
  {
    char Buf[200];
    LargeObjectAccess O(T, m_Object, ios::in);
    Buf[O.read(Buf, sizeof(Buf)-1)] = '\0';
    if (Contents != Buf)
      throw runtime_error("Expected large object #" + 
	                  ToString(m_Object.id()) + " "
			  "to contain '" + Contents + "', "
			  "but found '" + Buf + "'");
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


// Test program for libpqxx: import file to large object
//
// Usage: test53 [connect-string]
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

    C.Perform(ImportLargeObject(Obj, "pqxxlo.txt"));
    C.Perform(ReadLargeObject(Obj));
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


