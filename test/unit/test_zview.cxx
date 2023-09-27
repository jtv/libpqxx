#include <pqxx/zview>

#include "../test_helpers.hxx"


namespace
{
void test_zview_literal()
{
  using pqxx::operator"" _zv;

  PQXX_CHECK_EQUAL(("foo"_zv), pqxx::zview{"foo"}, "zview literal is broken.");
}


void test_zview_converts_to_string()
{
  using pqxx::operator"" _zv;
  using traits = pqxx::string_traits<pqxx::zview>;

  PQXX_CHECK_EQUAL(
    pqxx::to_string("hello"_zv), std::string{"hello"},
    "to_string on zview failed.");

  char buf[100];

  auto const v{traits::to_buf(std::begin(buf), std::end(buf), "myview"_zv)};
  PQXX_CHECK_EQUAL(std::string{v}, "myview", "to_buf on zview failed.");

  auto const p{
    traits::into_buf(std::begin(buf), std::end(buf), "moreview"_zv)};
  PQXX_CHECK_NOT_EQUAL(
    p, std::begin(buf), "into_buf on zview returns beginning of buffer.");
  PQXX_CHECK(
    p > std::begin(buf) and p < std::end(buf),
    "into_buf on zview did not store in buffer.");
  PQXX_CHECK(*(p - 1) == '\0', "into_buf on zview wasted space.");
  PQXX_CHECK(*(p - 2) == 'w', "into_buf on zview has extraneous data.");
  PQXX_CHECK_EQUAL(std::string(buf), "moreview", "into_buf on zview failed.");
}


PQXX_REGISTER_TEST(test_zview_literal);
PQXX_REGISTER_TEST(test_zview_converts_to_string);
} // namespace
