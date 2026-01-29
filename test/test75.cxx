#include <vector>

#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Compare const_reverse_iterator iteration of a
// result to a regular, const_iterator iteration.
namespace
{
void test_075(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  pqxx::test::create_pqxxevents(tx);
  auto const R(tx.exec("SELECT year FROM pqxxevents"));
  PQXX_CHECK(not std::empty(R), "No events found, cannot test.");

  PQXX_CHECK(R[0] == R.at(0));
  PQXX_CHECK(not(R[0] != R.at(0)));

  PQXX_CHECK(R[0][0] == R[0].at(0));
  PQXX_CHECK(not(R[0][0] != R[0].at(0)));

  std::vector<std::string> contents;
  for (auto const &i : R) contents.push_back(i.at(0).as<std::string>());

  PQXX_CHECK_EQUAL(
    std::size(contents), std::vector<std::string>::size_type(std::size(R)));

  for (pqxx::result::size_type i{0}; i < std::size(R); ++i)
    PQXX_CHECK_EQUAL(
      contents[static_cast<std::size_t>(i)], R.at(i).at(0).c_str());

  // Thorough test for result::const_reverse_iterator
  pqxx::result::const_reverse_iterator const ri1(std::rbegin(R));
  pqxx::result::const_reverse_iterator ri2(ri1), ri3(std::end(R));
  ri2 = std::rbegin(R);

  PQXX_CHECK(ri2 == ri1);
  PQXX_CHECK(ri3 == ri2);
  PQXX_CHECK_EQUAL(ri2 - ri3, 0);

  PQXX_CHECK(ri2 == ri3 + 0);
  PQXX_CHECK(ri2 == ri3 - 0);
  PQXX_CHECK(not(ri3 < ri2));
  PQXX_CHECK(ri2 <= ri3);

  PQXX_CHECK(ri3++ == ri2);

  PQXX_CHECK_EQUAL(ri3 - ri2, 1);
  PQXX_CHECK(ri3 > ri2);
  PQXX_CHECK(ri3 >= ri2);
  PQXX_CHECK(ri2 < ri3);
  PQXX_CHECK(ri2 <= ri3);
  PQXX_CHECK(ri3 == ri2 + 1);
  PQXX_CHECK(ri2 == ri3 - 1);

  PQXX_CHECK(ri3 == ++ri2);
  PQXX_CHECK(ri3 >= ri2);
  PQXX_CHECK(ri3 >= ri2);

  PQXX_CHECK_EQUAL(ri3.base()->front().view(), R.back()[0].view());

  PQXX_CHECK_EQUAL(
    ri1->at(0).as<std::string>(), (*ri1).at(0).as<std::string>());

  PQXX_CHECK(ri2-- == ri3);
  PQXX_CHECK(ri2 == --ri3);
  PQXX_CHECK(ri2 == std::rbegin(R));

  ri2 += 1;
  ri3 -= -1;

  PQXX_CHECK(ri2 != std::rbegin(R));
  PQXX_CHECK(ri3 == ri2);

  ri2 -= 1;
  PQXX_CHECK(ri2 == std::rbegin(R));

  // Now verify that reverse iterator also sees the same results...
  auto l{std::rbegin(contents)};
  for (auto i{std::rbegin(R)}; i != std::rend(R); ++i, ++l)
    PQXX_CHECK_EQUAL(*l, i->at(0).c_str());

  PQXX_CHECK(l == std::rend(contents));

  PQXX_CHECK(not std::empty(R), "No events found in table, cannot test.");
}
} // namespace


PQXX_REGISTER_TEST(test_075);
