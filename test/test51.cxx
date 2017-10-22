#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace std;
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
    m_object(),
    m_object_output(O)
  {
  }

  void operator()(argument_type &T)
  {
    largeobjectaccess A(T);
    cout << "Created large object #" << A.id() << endl;
    m_object = largeobject(A);

    A.write(Contents);

    char Buf[200];
    const auto Size = sizeof(Buf) - 1;

    auto Offset = A.seek(0, ios::beg);
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

    using lobj_size_t = largeobjectaccess::size_type;
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
	m_object_output != m_object,
	"Large objects: false negative on operator!=().");

    PQXX_CHECK(
	!(m_object_output == m_object),
	"Large objects: false positive on operator==().");

    m_object_output = m_object;

    PQXX_CHECK(
	!(m_object_output != m_object),
	"Large objects: false positive on operator!=().");
    PQXX_CHECK(
	m_object_output == m_object,
	"Large objects: false negative on operator==().");

    PQXX_CHECK(
	m_object_output <= m_object,
	"Large objects: false negative on operator<=().");
    PQXX_CHECK(
	m_object_output >= m_object,
	"Large objects: false negative on operator>=().");

    PQXX_CHECK(
	!(m_object_output < m_object),
	"Large objects: false positive on operator<().");
    PQXX_CHECK(
	!(m_object_output > m_object),
	"Large objects: false positive on operator>().");
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
