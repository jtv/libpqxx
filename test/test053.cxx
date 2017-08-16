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
    m_object(O),
    m_file(File)
  {
  }

  void operator()(argument_type &T)
  {
    m_object = largeobject(T, m_file);
    cout << "Imported '" << m_file << "' "
            "to large object #" << m_object.id() << endl;
  }

private:
  largeobject &m_object;
  string m_file;
};


class ReadLargeObject : public transactor<>
{
public:
  explicit ReadLargeObject(largeobject &O) :
    transactor<>("ReadLargeObject"),
    m_object(O)
  {
  }

  void operator()(argument_type &T)
  {
    char Buf[200];
    largeobjectaccess O(T, m_object, ios::in);
    const auto len = O.read(Buf, sizeof(Buf)-1);
    PQXX_CHECK_EQUAL(
	string(Buf, string::size_type(len)),
	Contents,
	"Large object contents were mangled.");
  }

private:
  largeobject m_object;
};


class DeleteLargeObject : public transactor<>
{
public:
  explicit DeleteLargeObject(largeobject O) : m_object(O) {}

  void operator()(argument_type &T)
  {
    m_object.remove(T);
  }

private:
  largeobject m_object;
};


void test_053(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj;

  C.perform(ImportLargeObject(Obj, "pqxxlo.txt"));
  C.perform(ReadLargeObject(Obj));
  C.perform(DeleteLargeObject(Obj));
}
} // namespace

PQXX_REGISTER_TEST_T(test_053, nontransaction)
