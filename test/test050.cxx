#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx's Large Objects interface.
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
    largeobjectaccess A(T, m_Object);
    const largeobjectaccess::pos_type
	orgpos = A.ctell(),
	copyorgpos = A.ctell();

    PQXX_CHECK_EQUAL(orgpos, 0, "Bad initial position in large object.");
    PQXX_CHECK_EQUAL(copyorgpos, orgpos, "ctell() affected positioning.");

    const largeobjectaccess::pos_type cxxorgpos = A.tell();
    PQXX_CHECK_EQUAL(cxxorgpos, orgpos, "tell() reports bad position.");

    A.process_notice("Writing to large object #" +
	to_string(largeobject(A).id()) + "\n");
    long Bytes = A.cwrite(
	Contents.c_str(),
	largeobject::size_type(Contents.size()));

    PQXX_CHECK_EQUAL(
	Bytes,
	long(Contents.size()),
	"Wrote wrong number of bytes.");

    PQXX_CHECK_EQUAL(
	A.tell(),
	A.ctell(),
	"tell() is inconsistent with ctell().");

    PQXX_CHECK_EQUAL(A.tell(), Bytes, "Bad large-object position.");

    char Buf[200];
    const size_t Size = sizeof(Buf) - 1;
    PQXX_CHECK_EQUAL(
	A.cread(Buf, Size),
	0,
	"Bad return value from cread() after writing.");

    PQXX_CHECK_EQUAL(
	size_t(A.cseek(0, ios::cur)),
	Contents.size(),
	"Unexpected position after cseek(0, cur).");

    PQXX_CHECK_EQUAL(
	A.cseek(1, ios::beg),
	1,
	"Unexpected cseek() result after seeking to position 1.");

    PQXX_CHECK_EQUAL(
	A.cseek(-1, ios::cur),
	0,
	"Unexpected cseek() result after seeking -1 from position 1.");

    PQXX_CHECK(size_t(A.read(Buf, Size)) <= Size, "Got too many bytes.");

    PQXX_CHECK_EQUAL(
	string(Buf, string::size_type(Bytes)),
	Contents,
	"Large-object contents were mutilated.");
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


void test_050(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj;

  C.perform(CreateLargeObject(Obj));
  C.perform(WriteLargeObject(Obj));
  C.perform(DeleteLargeObject(Obj));
}
} // namespace

PQXX_REGISTER_TEST_T(test_050, nontransaction)
