#include "helpers.hxx"

#include "pqxx/internal/encodings.hxx"


namespace
{
/// Convenience shorthand: `std::source_location::current()`.
pqxx::sl loc(pqxx::sl l = pqxx::sl::current())
{
  return l;
}


void test_find_chars()
{
  auto finder{pqxx::internal::get_char_finder<'.', '!', '?'>(
    pqxx::encoding_group::ascii_safe, loc())};

  PQXX_CHECK_EQUAL(finder("", 0, loc()), 0u);
  PQXX_CHECK_EQUAL(finder("...", 0, loc()), 0u);
  PQXX_CHECK_EQUAL(finder("...", 1, loc()), 1u);
  PQXX_CHECK_EQUAL(finder("abcd", 0, loc()), 4u);
  PQXX_CHECK_EQUAL(finder("hello?", 0, loc()), 5u);
}


PQXX_REGISTER_TEST(test_find_chars);
} // namespace
