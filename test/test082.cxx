#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Test program for libpqxx.  Read and print table using row iterators.
namespace
{
void test_082(transaction_base &T)
{
  test::create_pqxxevents(T);
  const string Table = "pqxxevents";
  result R( T.exec("SELECT * FROM " + Table) );
  T.conn().disconnect();

  PQXX_CHECK(!R.empty(), "Got empty result.");

  const string nullstr("[null]");

  for (pqxx::row::const_iterator f = R[0].begin(); f != R[0].end(); ++f)
    cout << f->name() << '\t';
  cout << endl << endl;
  for (result::const_iterator r = R.begin(); r != R.end(); ++r)
  {
    pqxx::row::const_iterator f2(r[0]);
    for (pqxx::row::const_iterator f=r->begin(); f!=r->end(); ++f, f2++)
    {
      cout << f->c_str() << '\t';
      PQXX_CHECK_EQUAL(
	(*f2).as(nullstr),
	f->as(nullstr),
	"Inconsistent iteration result.");
    }

    PQXX_CHECK(
	r->begin() + pqxx::row::difference_type(r->size()) == r->end(),
	"Row end() appears to be in the wrong place.");
    PQXX_CHECK(
	pqxx::row::difference_type(r->size()) + r->begin() == r->end(),
	"Row iterator addition is not commutative.");
    PQXX_CHECK_EQUAL(r->begin()->num(), 0u, "Wrong column number at begin().");

    pqxx::row::const_iterator f3(r[r->size()]);

    PQXX_CHECK(f3 == r->end(), "Did not get end() at end of row.");

    PQXX_CHECK(f3 > r->begin(), "Row end() appears to precede its begin().");

    PQXX_CHECK(
	f3 >= r->end() && r->begin() < f3,
	"Row iterator operator<() is broken.");

    PQXX_CHECK(f3 > r->begin(), "Row end() not greater than begin().");

    pqxx::row::const_iterator f4(*r, r->size());
    PQXX_CHECK(f4 == f3, "Row iterator constructor with offset is broken.");

    f3--;
    f4 -= 1;

    PQXX_CHECK(f3 < r->end(), "Last field in row is not before end().");
    PQXX_CHECK(f3 >= r.begin(), "Last field in row precedes begin().");
    PQXX_CHECK(f3 == r.end()-1, "Back from end() doese not yield end()-1.");
    PQXX_CHECK_EQUAL(
	r->end() - f3,
	1,
	"Wrong distance from last row to end().");

    PQXX_CHECK(f4 == f3, "Row iterator operator-=() is broken.");
    f4 += 1;
    PQXX_CHECK(f4 == r->end(), "Row iterator operator+=() is broken.");

    for (pqxx::row::const_reverse_iterator fr = r->rbegin();
	 fr != r->rend();
	 ++fr, --f3)
      PQXX_CHECK_EQUAL(
	*fr,
	*f3,
	"Reverse traversal is not consistent with forward traversal.");

    cout <<endl;
  }

  // Thorough test for row::const_reverse_iterator
  pqxx::row::const_reverse_iterator
    ri1(R.front().rbegin()), ri2(ri1), ri3(R.front().end());
  ri2 = R.front().rbegin();

  PQXX_CHECK(
	ri1 == ri2,
	"Copy-constructed reverse_iterator is not equal to original.");

  PQXX_CHECK(ri2 == ri3, "result::end() does not generate rbegin().");
  PQXX_CHECK_EQUAL(
	ri2 - ri3,
	0,
	"Distance between identical const_reverse_iterators was nonzero.");

  PQXX_CHECK(
	pqxx::row::const_reverse_iterator(ri1.base()) == ri1,
	"Back-conversion of reverse_iterator base() fails.");

  PQXX_CHECK(ri2 == ri3 + 0, "reverse_iterator+0 gives strange result.");
  PQXX_CHECK(ri2 == ri3 - 0, "reverse_iterator-0 gives strange result.");

  PQXX_CHECK(
	!(ri3 < ri2),
	"reverse_iterator operator<() breaks on identical iterators.");
  PQXX_CHECK(
	ri2 <= ri3,
	"reverse_iterator operator<=() breaks on identical iterators.");
  PQXX_CHECK(
	ri3++ == ri2,
	"reverse_iterator post-increment is broken.");

  PQXX_CHECK_EQUAL(ri3 - ri2, 1, "Wrong reverse_iterator distance.");
  PQXX_CHECK(ri3 > ri2, "reverse_iterator operator>() is broken.");
  PQXX_CHECK(ri3 >= ri2, "reverse_iterator operator>=() is broken.");
  PQXX_CHECK(ri2 < ri3, "reverse_iterator operator<() is broken.");
  PQXX_CHECK(ri2 <= ri3, "reverse_iterator operator<=() is broken.");
  PQXX_CHECK(ri3 == ri2 + 1, "Adding number to reverse_iterator goes wrong.");
  PQXX_CHECK(ri2 == ri3 - 1, "Subtracting from reverse_iterator goes wrong.");

  PQXX_CHECK(
	ri3 == ++ri2,
	"reverse_iterator pre-incremen returns wrong result.");

  PQXX_CHECK(
	ri3 >= ri2,
	"reverse_iterator operator>=() breaks on equal iterators.");
  PQXX_CHECK(
	ri3 >= ri2,
	"reverse_iterator operator<=() breaks on equal iterators.");
  PQXX_CHECK(
	ri3.base() == R.front().back(),
	"reverse_iterator does not arrive at back().");
  PQXX_CHECK(
	ri1->c_str()[0] == (*ri1).c_str()[0],
	"reverse_iterator operator->() is inconsistent with operator*().");
  PQXX_CHECK(
	ri2-- == ri3,
	"reverse_iterator post-decrement returns wrong result.");
  PQXX_CHECK(
	ri2 == --ri3,
	"reverse_iterator pre-increment returns wrong result.");
  PQXX_CHECK(
	ri2 == R.front().rbegin(),
	"Moving iterator back and forth doesn't get it back to origin.");

  ri2 += 1;
  ri3 -= -1;

  PQXX_CHECK(
	ri2 != R.front().rbegin(),
	"Adding to reverse_iterator doesn't work.");
  PQXX_CHECK(
	ri2 != R.front().rbegin(),
	"Adding to reverse_iterator doesn't work.");
  PQXX_CHECK(
	ri3 == ri2,
	"reverse_iterator operator-=() breaks on negative numbers.");

  ri2 -= 1;
  PQXX_CHECK(
	ri2 == R.front().rbegin(),
	"reverse_iterator operator+=() and operator-=() do not cancel out");
}
} // namespace

PQXX_REGISTER_TEST_T(test_082, nontransaction)
