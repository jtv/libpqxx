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
    pqxx::encoding_group::monobyte, loc())};

  PQXX_CHECK_EQUAL(
    finder("", 0, loc()), 0u, "Found something in empty string!?");
  PQXX_CHECK_EQUAL(
    finder("...", 0, loc()), 0u, "Did not stop at first match.");
  PQXX_CHECK_EQUAL(
    finder("...", 1, loc()), 1u, "Did not start at indicated position.");
  PQXX_CHECK_EQUAL(
    finder("abcd", 0, loc()), 4u, "Did not scan to end of string.");
  PQXX_CHECK_EQUAL(
    finder("hello?", 0, loc()), 5u, "Did not find match at end.");
}


PQXX_REGISTER_TEST(test_find_chars);
} // namespace
