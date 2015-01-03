#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx: import file to large object
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
    const largeobjectaccess::size_type len = A.read(Buf, sizeof(Buf)-1);
    PQXX_CHECK_EQUAL(
	string(Buf, string::size_type(len)),
	Contents,
	"Large object contents were mangled.");
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


void test_055(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj;

  C.perform(ImportLargeObject(Obj, "pqxxlo.txt"));
  C.perform(DeleteLargeObject(Obj));
}
} // namespace

PQXX_REGISTER_TEST_T(test_055, nontransaction)
