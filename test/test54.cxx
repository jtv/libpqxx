#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx: write large object to test files.
namespace
{
const string Contents = "Large object test contents";


class CreateLargeObject : public transactor<>
{
public:
  explicit CreateLargeObject(largeobject &O) :
    transactor<>("CreateLargeObject"),
    m_object(),
    m_object_output(O)
  {
  }

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T);
    m_object = largeobject(A);
    cout << "Created large object #" << m_object.id() << endl;
    A.write(Contents);
    A.to_file("pqxxlo.txt");
  }

  void on_commit()
  {
    m_object_output = m_object;
  }

private:
  largeobject m_object;
  largeobject &m_object_output;
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


void test_054(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj;

  C.perform(CreateLargeObject(Obj));
  C.perform(DeleteLargeObject(Obj));
}
} // namespace

PQXX_REGISTER_TEST_T(test_054, nontransaction)
