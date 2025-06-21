#include <ranges>

#include <pqxx/zview>

#include "helpers.hxx"


namespace
{
void test_zview_is_a_range()
{
  static_assert(std::ranges::range<pqxx::zview>);
  static_assert(std::ranges::borrowed_range<pqxx::zview>);
  static_assert(std::ranges::contiguous_range<pqxx::zview>);
}


void test_zview_literal()
{
  using pqxx::operator""_zv;

  PQXX_CHECK_EQUAL(("foo"_zv), pqxx::zview{"foo"});
}


void test_zview_converts_to_string()
{
  using pqxx::operator""_zv;
  using traits = pqxx::string_traits<pqxx::zview>;

  PQXX_CHECK_EQUAL(pqxx::to_string("hello"_zv), std::string{"hello"});

  char buf[100];

  auto const v{traits::to_buf(std::begin(buf), std::end(buf), "myview"_zv)};
  PQXX_CHECK_EQUAL(std::string{v}, "myview");

  auto const p{
    traits::into_buf(std::begin(buf), std::end(buf), "moreview"_zv)};
  PQXX_CHECK_NOT_EQUAL(
    p, std::begin(buf), "into_buf on zview returns beginning of buffer.");
  PQXX_CHECK(
    p > std::begin(buf) and p < std::end(buf),
    "into_buf on zview did not store in buffer.");
  PQXX_CHECK(*(p - 1) == '\0', "into_buf on zview wasted space.");
  PQXX_CHECK(*(p - 2) == 'w');
  PQXX_CHECK_EQUAL(std::string(std::data(buf)), "moreview");
}


PQXX_REGISTER_TEST(test_zview_is_a_range);
PQXX_REGISTER_TEST(test_zview_literal);
PQXX_REGISTER_TEST(test_zview_converts_to_string);
} // namespace
