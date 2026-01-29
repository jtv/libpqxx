#include <pqxx/separated_list>

#include "helpers.hxx"

// Test program for separated_list.

namespace
{
void test_separated_list(pqxx::test::context &)
{
  PQXX_CHECK_EQUAL(pqxx::separated_list(",", std::vector<int>{}), "");
  PQXX_CHECK_EQUAL(pqxx::separated_list(",", std::vector<int>{5}), "5");
  PQXX_CHECK_EQUAL(pqxx::separated_list(",", std::vector<int>{3, 6}), "3,6");

  std::vector<int> const nums{1, 2, 3};
  PQXX_CHECK_EQUAL(
    pqxx::separated_list(
      "+", std::begin(nums), std::end(nums),
      [](auto elt) { return *elt * 2; }),
    "2+4+6", "Accessors don't seem to work.");
}


PQXX_REGISTER_TEST(test_separated_list);
} // namespace
