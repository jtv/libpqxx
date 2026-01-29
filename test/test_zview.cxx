#include <ranges>

#include <pqxx/zview>

#include "helpers.hxx"


namespace
{
void test_zview_is_a_range(pqxx::test::context &)
{
  static_assert(std::ranges::range<pqxx::zview>);
  static_assert(std::ranges::borrowed_range<pqxx::zview>);
  static_assert(std::ranges::contiguous_range<pqxx::zview>);
}


void test_zview_literal(pqxx::test::context &)
{
  using pqxx::operator""_zv;

  PQXX_CHECK_EQUAL(("foo"_zv), pqxx::zview{"foo"});
}


void test_zview_converts_to_string(pqxx::test::context &)
{
  using pqxx::operator""_zv;
  using traits = pqxx::string_traits<pqxx::zview>;

  PQXX_CHECK_EQUAL(pqxx::to_string("hello"_zv), (std::string{"hello"}));

  std::array<char, 100> buf{};

  auto const v{traits::to_buf(buf, "myview"_zv)};
  PQXX_CHECK_EQUAL((std::string{v}), "myview");

  auto const p{pqxx::into_buf(buf, "moreview"_zv)};
  PQXX_CHECK(
    p == std::strlen("moreview"),
    "into_buf of zview did not store in buffer.");
  PQXX_CHECK(buf.at(p - 1) == 'w');
  PQXX_CHECK_EQUAL((std::string{std::data(buf), p}), "moreview");
}


void test_zview_properties(pqxx::test::context &tctx)
{
  auto const str{tctx.make_name("zv")};
  PQXX_CHECK_EQUAL(
    (pqxx::zview{str.c_str(), std::size(str)}), (pqxx::zview{str}));
  PQXX_CHECK_EQUAL((pqxx::zview{str.c_str()}), (pqxx::zview{str}));
  PQXX_CHECK_EQUAL((pqxx::zview{std::string_view{str}}), (pqxx::zview{str}));
  PQXX_CHECK_EQUAL((std::string{pqxx::zview{str}.c_str()}), str);
}


PQXX_REGISTER_TEST(test_zview_is_a_range);
PQXX_REGISTER_TEST(test_zview_literal);
PQXX_REGISTER_TEST(test_zview_converts_to_string);
PQXX_REGISTER_TEST(test_zview_properties);
} // namespace
