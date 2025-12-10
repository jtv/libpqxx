#include <pqxx/zview>

#include "../test_helpers.hxx"


namespace
{
void test_zview_literal()
{
  using pqxx::operator""_zv;

  PQXX_CHECK_EQUAL(("foo"_zv), pqxx::zview{"foo"}, "zview literal is broken.");
}


void test_zview_converts_to_string()
{
  using pqxx::operator""_zv;
  using traits = pqxx::string_traits<pqxx::zview>;

  PQXX_CHECK_EQUAL(
    pqxx::to_string("hello"_zv), std::string{"hello"},
    "to_string on zview failed.");

  std::array<char, 100> buf;
  auto const buf_begin{std::data(buf)};
  auto const buf_end{buf_begin + std::size(buf)};

  auto const v{traits::to_buf(buf_begin, buf_end, "myview"_zv)};
  PQXX_CHECK_EQUAL(std::string{v}, "myview", "to_buf on zview failed.");

  auto const p{traits::into_buf(buf_begin, buf_end, "moreview"_zv)};
  PQXX_CHECK_NOT_EQUAL(
    p, buf_begin, "into_buf on zview returns beginning of buffer.");
  PQXX_CHECK(
    p > buf_begin and p < buf_end,
    "into_buf on zview did not store in buffer.");
  PQXX_CHECK(*(p - 1) == '\0', "into_buf on zview wasted space.");
  PQXX_CHECK(*(p - 2) == 'w', "into_buf on zview has extraneous data.");
  PQXX_CHECK_EQUAL(
    std::string(buf_begin), "moreview", "into_buf on zview failed.");
}


PQXX_REGISTER_TEST(test_zview_literal);
PQXX_REGISTER_TEST(test_zview_converts_to_string);
} // namespace
