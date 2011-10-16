#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx's Large Objects interface.
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

    typedef largeobjectaccess::size_type lobj_size_t;
    char Buf[200];
    const lobj_size_t Size = sizeof(Buf) - 1;

    lobj_size_t Offset = A.seek(0, ios::beg);
    PQXX_CHECK_EQUAL(Offset, 0, "Wrong position after seek to beginning.");

    PQXX_CHECK_EQUAL(
	size_t(A.read(Buf, Size)),
	Contents.size(),
	"Unexpected read() result.");

    PQXX_CHECK_EQUAL(
	string(Buf, Contents.size()),
	Contents,
	"Large object contents were mutilated.");

    // Now write contents again, this time as a C string
    PQXX_CHECK_EQUAL(
	A.seek(-int(Contents.size()), ios::end),
	0,
	"Bad position after seeking to beginning of large object.");

    A.write(Buf, lobj_size_t(Contents.size()));
    A.seek(0, ios::beg);
    PQXX_CHECK_EQUAL(
	size_t(A.read(Buf, Size)),
	Contents.size(),
	"Bad length for rewritten large object.");

    PQXX_CHECK_EQUAL(
	string(Buf, Contents.size()),
	Contents,
	"Rewritten large object was mangled.");
  }

  void on_commit()
  {
    PQXX_CHECK(
	m_ObjectOutput != m_Object,
	"Large objects: false negative on operator!=().");

    PQXX_CHECK(
	!(m_ObjectOutput == m_Object),
	"Large objects: false positive on operator==().");

    m_ObjectOutput = m_Object;

    PQXX_CHECK(
	!(m_ObjectOutput != m_Object),
	"Large objects: false positive on operator!=().");
    PQXX_CHECK(
	m_ObjectOutput == m_Object,
	"Large objects: false negative on operator==().");

    PQXX_CHECK(
	m_ObjectOutput <= m_Object,
	"Large objects: false negative on operator<=().");
    PQXX_CHECK(
	m_ObjectOutput >= m_Object,
	"Large objects: false negative on operator>=().");

    PQXX_CHECK(
	!(m_ObjectOutput < m_Object),
	"Large objects: false positive on operator<().");
    PQXX_CHECK(
	!(m_ObjectOutput > m_Object),
	"Large objects: false positive on operator>().");
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


void test_051(transaction_base &orgT)
{
  connection_base &C(orgT.conn());
  orgT.abort();

  largeobject Obj;

  C.perform(WriteLargeObject(Obj));
  C.perform(DeleteLargeObject(Obj));
}
} // namespace

PQXX_REGISTER_TEST_T(test_051, nontransaction)
