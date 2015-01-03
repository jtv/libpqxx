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
    m_Object(),
    m_ObjectOutput(O)
  {
  }

  void operator()(argument_type &T)
  {
    m_Object = largeobject(T);
    cout << "Created large object #" << m_Object.id() << endl;
  }

  void on_commit()
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
    largeobjectaccess A(T, m_Object.id(), ios::out);
    cout << "Writing to large object #" << largeobject(A).id() << endl;
    A.write(Contents);
  }

private:
  largeobject m_Object;
};


class CopyLargeObject : public transactor<>
{
public:
  explicit CopyLargeObject(largeobject O) : m_Object(O) {}

  void operator()(argument_type &T)
  {
    m_Object.to_file(T, "pqxxlo.txt");
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
