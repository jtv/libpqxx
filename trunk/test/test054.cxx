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
    largeobjectaccess A(T);
    m_Object = largeobject(A);
    cout << "Created large object #" << m_Object.id() << endl;
    A.write(Contents);
    A.to_file("pqxxlo.txt");
  }

  void on_commit()
  {
    m_ObjectOutput = m_Object;
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


// Test program for libpqxx: write large object to test files.
//
// Usage: test054 [connect-string]
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


