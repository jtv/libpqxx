#include <iostream>
#include <sstream>

#include <pqxx/all.h>
#include <pqxx/largeobject.h>

using namespace PGSTD;
using namespace pqxx;

namespace
{

const string Contents = "Large object test contents";

class ImportLargeObject : public transactor<>
{
public:
  explicit ImportLargeObject(largeobject &O, const string &File) :
    transactor<>("ImportLargeObject"),
    m_Object(O),
    m_File(File)
  {
  }

  void operator()(argument_type &T)
  {

    largeobjectaccess A(T, "pqxxlo.txt", ios::in);
    m_Object = largeobject(A);
    cout << "Imported '" << m_File << "' "
            "to large object #" << m_Object.id() << endl;

    char Buf[200];
    Buf[A.read(Buf, sizeof(Buf)-1)] = '\0';
    if (Contents != Buf)
      throw runtime_error("Expected large object #" + 
	                  ToString(m_Object.id()) + " "
			  "to contain '" + Contents + "', "
			  "but found '" + Buf + "'");
  }

private:
  largeobject &m_Object;
  string m_File;
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


// Test program for libpqxx: import file to large object
//
// Usage: test055 [connect-string]
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

    C.Perform(ImportLargeObject(Obj, "pqxxlo.txt"));
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


