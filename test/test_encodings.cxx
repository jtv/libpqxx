#include "helpers.hxx"

#include "pqxx/internal/encodings.hxx"


namespace
{
void test_scan_ascii()
{
  auto const scan{pqxx::internal::get_glyph_scanner(
    pqxx::encoding_group::MONOBYTE, pqxx::sl::current())};
  std::string const text{"hello"};

  PQXX_CHECK_EQUAL(
    scan(text.c_str(), std::size(text), 0, pqxx::sl::current()), 1ul,
    "Monobyte scanner acting up.");
  PQXX_CHECK_EQUAL(
    scan(text.c_str(), std::size(text), 1, pqxx::sl::current()), 2ul,
    "Monobyte scanner is inconsistent.");
}


void test_scan_utf8()
{
  auto const scan{pqxx::internal::get_glyph_scanner(
    pqxx::encoding_group::UTF8, pqxx::sl::current())};

  // Thai: "Khrab".
  std::string const text{"\xe0\xb8\x95\xe0\xb8\xa3\xe0\xb8\xb1\xe0\xb8\x9a"};
  PQXX_CHECK_EQUAL(
    scan(text.c_str(), std::size(text), 0, pqxx::sl::current()), 3ul,
    "UTF-8 scanner mis-scanned Thai khor khwai.");
  PQXX_CHECK_EQUAL(
    scan(text.c_str(), std::size(text), 3, pqxx::sl::current()), 6ul,
    "UTF-8 scanner mis-scanned Thai ror reua.");
}


void test_encodings()
{
  test_scan_ascii();
  test_scan_utf8();
}


PQXX_REGISTER_TEST(test_encodings);
} // namespace
