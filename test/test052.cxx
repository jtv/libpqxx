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
    m_object = largeobject(T);
    cout << "Created large object #" << m_object.id() << endl;
  }

  void on_commit()
  {
    m_object_output = m_object;
  }

private:
  largeobject m_object;
  largeobject &m_object_output;
};


class WriteLargeObject : public transactor<>
{
public:
  explicit WriteLargeObject(largeobject &O) :
    transactor<>("WriteLargeObject"),
    m_object(O)
  {
  }

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T, m_object.id(), ios::out);
    cout << "Writing to large object #" << largeobject(A).id() << endl;
    A.write(Contents);
  }

private:
  largeobject m_object;
};


class CopyLargeObject : public transactor<>
{
public:
  explicit CopyLargeObject(largeobject O) : m_object(O) {}

  void operator()(argument_type &T)
  {
    m_object.to_file(T, "pqxxlo.txt");
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


void test_052(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj;

  C.perform(CreateLargeObject(Obj));
  C.perform(WriteLargeObject(Obj));
  C.perform(CopyLargeObject(Obj));
  C.perform(DeleteLargeObject(Obj));
}
} // namespace

PQXX_REGISTER_TEST_T(test_052, nontransaction)
