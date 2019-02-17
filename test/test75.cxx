#include <iostream>
#include <vector>

#include "test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx.  Compare const_reverse_iterator iteration of a
// result to a regular, const_iterator iteration.
namespace
{
void test_075()
{
  connection conn;
  work tx{conn};

  test::create_pqxxevents(tx);
  const result R( tx.exec("SELECT year FROM pqxxevents") );
  PQXX_CHECK(not R.empty(), "No events found, cannot test.");

  PQXX_CHECK_EQUAL(R[0], R.at(0), "Inconsistent result indexing.");
  PQXX_CHECK(not (R[0] != R.at(0)), "result::row::operator!=() is broken.");

  PQXX_CHECK_EQUAL(R[0][0], R[0].at(0), "Inconsistent row indexing.");
  PQXX_CHECK(
	not (R[0][0] != R[0].at(0)),
	"result::field::operator!=() is broken.");

  std::vector<std::string> contents;
  for (const auto &i: R) contents.push_back(i.at(0).as<std::string>());
  std::cout << to_string(contents.size()) << " years read" << std::endl;

  PQXX_CHECK_EQUAL(
	contents.size(),
	std::vector<std::string>::size_type(R.size()),
	"Number of values does not match result size.");

  for (result::size_type i=0; i<R.size(); ++i)
    PQXX_CHECK_EQUAL(
	contents[i],
	R.at(i).at(0).c_str(),
	"Inconsistent iteration.");

  std::cout << to_string(R.size()) << " years checked" << std::endl;

  // Thorough test for result::const_reverse_iterator
  result::const_reverse_iterator ri1(R.rbegin()), ri2(ri1), ri3(R.end());
  ri2 = R.rbegin();

  PQXX_CHECK(ri2 == ri1, "reverse_iterator copy constructor is broken.");
  PQXX_CHECK(ri3 == ri2, "result::end() does not generate rbegin().");
  PQXX_CHECK_EQUAL(
	ri2 - ri3,
	0,
	"const_reverse_iterator is at nonzero distance from its own copy.");

  PQXX_CHECK(ri2 == ri3 + 0, "reverse_iterator+0 gives strange result.");
  PQXX_CHECK(ri2 == ri3 - 0, "reverse_iterator-0 gives strange result.");
  PQXX_CHECK(not (ri3 < ri2), "operator<() breaks on equal reverse_iterators.");
  PQXX_CHECK(ri2 <= ri3, "operator<=() breaks on equal reverse_iterators.");

  PQXX_CHECK(ri3++ == ri2, "reverse_iterator post-increment is broken.");

  PQXX_CHECK_EQUAL(ri3 - ri2, 1, "Wrong nonzero reverse_iterator distance.");
  PQXX_CHECK(ri3 > ri2, "reverse_iterator operator>() is broken.");
  PQXX_CHECK(ri3 >= ri2, "reverse_iterator operator>=() is broken.");
  PQXX_CHECK(ri2 < ri3, "reverse_iterator operator<() is broken.");
  PQXX_CHECK(ri2 <= ri3, "reverse_iterator operator<=() is broken.");
  PQXX_CHECK(ri3 == ri2 + 1, "Adding int to reverse_iterator is broken.");
  PQXX_CHECK(
	ri2 == ri3 - 1,
	"Subtracting int from reverse_iterator is broken.");

  PQXX_CHECK(ri3 == ++ri2, "reverse_iterator pre-increment is broken.");
  PQXX_CHECK(ri3 >= ri2, "operator>=() breaks on equal reverse_iterators.");
  PQXX_CHECK(ri3 >= ri2, "operator<=() breaks on equal reverse_iterators.");

  PQXX_CHECK(
	ri3.base() == R.back(),
	"reverse_iterator does not arrive at back().");

  PQXX_CHECK(
	ri1->at(0) == (*ri1).at(0),
	"reverse_iterator operator->() is inconsistent with operator*().");

  PQXX_CHECK(ri2-- == ri3, "reverse_iterator post-decrement is broken.");
  PQXX_CHECK(ri2 == --ri3, "reverse_iterator pre-decrement is broken.");
  PQXX_CHECK(ri2 == R.rbegin(), "reverse_iterator decrement is broken.");

  ri2 += 1;
  ri3 -= -1;

  PQXX_CHECK(ri2 != R.rbegin(), "Adding to reverse_iterator does not work.");
  PQXX_CHECK(
	ri3 == ri2,
	"reverse_iterator operator-=() breaks on negative distances.");

  ri2 -= 1;
  PQXX_CHECK(
	ri2 == R.rbegin(),
	"reverse_iterator operator+=() and operator-=() do not cancel out.");

  // Now verify that reverse iterator also sees the same results...
  auto l = contents.rbegin();
  for (auto i = R.rbegin(); i != R.rend(); ++i, ++l)
    PQXX_CHECK_EQUAL(
	*l,
	i->at(0).c_str(),
	"Inconsistent reverse iteration.");

  PQXX_CHECK(l == contents.rend(), "Reverse iteration ended too soon.");

  PQXX_CHECK(not R.empty(), "No events found in table, cannot test.");
}
} // namespace


PQXX_REGISTER_TEST(test_075);
