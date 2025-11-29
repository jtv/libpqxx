#include "helpers.hxx"

#include "pqxx/internal/encodings.hxx"


namespace
{
/// Convenience shorthand: `std::source_location::current()`.
pqxx::sl loc(pqxx::sl l = pqxx::sl::current())
{
  return l;
}


/// Test basic sanity of search in encoding group `ENC`.
/** Searches `text` for first occurrence of 'X'.
 *
 * Ensure that `text` does not contain the ASCII _character_ X.  However, it
 * may contain the _byte value_ for that character.  In fact it makes the test
 * stronger and more useful.
 */
template<pqxx::encoding_group ENC>
void test_search(std::string_view text, std::string_view enc_name)
{
  auto const finder{pqxx::internal::get_char_finder<'X'>(ENC, loc())};

  // First, we do some generic tests on ASCII strings.  All supported
  // encodings are ASCII supersets, so a plain ASCII string is valid and
  // correct in each of them.
  PQXX_CHECK_EQUAL(
    finder("", 0, loc()), 0u,
    std::format("Empty string search ({}) went out of bounds.", enc_name));
  PQXX_CHECK_EQUAL(
    finder("XXX", 0, loc()), 0u,
    std::format(
      "Search on ASCII string ({}) missed starting char.", enc_name));
  PQXX_CHECK_EQUAL(
    finder("XXX", 1, loc()), 1u,
    std::format(
      "Search ({}) at non-zero offset ended in the wrong place.", enc_name));
  PQXX_CHECK_EQUAL(
    finder("abcd", 0, loc()), 4u,
    std::format(
      "Search ({}) for absent character in ASCII string went wrong.",
      enc_name));

  // Now try searching the actual `text`.  First a failing search, since `text`
  // does not contain the character we're looking for:
  PQXX_CHECK_EQUAL(
    finder(text, 0, loc()), std::size(text),
    "Search for absent character did not hit end.");
  // Then, a successful search.
  PQXX_CHECK_EQUAL(
    finder(std::string{text} + "Xnn", 0, loc()), std::size(text),
    "False negative on search.");
}


void test_find_chars_big5()
{
  // "My hovercraft is full of eels" in Traditional Chinese (as translated by
  // Google Translate), encoded to big5.
  std::string_view const clear_cut{
    "\xa7\xda\xaa\xba\xae\xf0\xb9\xd4\xb2\xee\xb8\xcc\xa5\xfe\xac\x4f\xc5\xc1"
    "\xb3\xbd"};

  test_search<pqxx::encoding_group::big5>(clear_cut, "big5");

  // This character's second byte has the same numeric value as the ASCII
  // character 'X'.
  test_search<pqxx::encoding_group::big5>("\xaa\x58", "big5");
}


void test_find_chars_ascii()
{
  test_search<pqxx::encoding_group::ascii_safe>("some text", "ascii_safe");
}


void test_find_chars_gb18030()
{
  // "My hovercraft is full of eels" in Simplified Chinese (as translated by
  // Google Translate), encoded to gb18030.
  std::string_view const clear_cut{
    "\xce\xd2\xb5\xc4\xc6\xf8\xb5\xe6\xb4\xac\xc0\xef\xd7\xb0\xc2\xfa\xc1\xcb"
    "\xf7\xa9\xd3\xe3\xa1\xa3"};

  // Straightforward gb18030 string.
  test_search<pqxx::encoding_group::gb18030>(clear_cut, "gb18030");

  // This character's second byte has the same numeric value as the ASCII
  // character 'X'.
  test_search<pqxx::encoding_group::gb18030>("\x81\x58", "gb18030");
}


void test_find_chars_gbk()
{
  // "My hovercraft is full of eels" in Simplified Chinese (as translated by
  // Google Translate), encoded to gb18030.
  std::string_view const clear_cut{
    "\xce\xd2\xb5\xc4\xc6\xf8\xb5\xe6\xb4\xac\xc0\xef\xd7\xb0\xc2\xfa\xc1\xcb"
    "\xf7\xa9\xd3\xe3\xa1\xa3"};

  test_search<pqxx::encoding_group::gbk>(clear_cut, "gbk");

  // This character's second byte has the same numeric value as the ASCII
  // character 'X'.
  test_search<pqxx::encoding_group::gbk>("\xaa\x58", "gbk");
}


void test_find_chars_johab()
{
  // "My hovercraft is full of eels" in Korean (as translated by Google
  // Translate), encoded to JOHAB.
  std::string_view const clear_cut{
    "\x90\x81 \xd1\xa1\xa4\xe1\xc7\x61\x9c\x81\xc\x66\x61\xc\x62\x61\x93\x65 "
    "\xb8\x77\xb4\xe1\x9d\xa1 \x88\x61\x97\x62 \xc0\x61 "
    "\xb7\xb6\xb4\xe1\xb6\x61"};

  test_search<pqxx::encoding_group::johab>(clear_cut, "johab");

  // This character's second byte has the same numeric value as the ASCII
  // character 'X'.
  test_search<pqxx::encoding_group::johab>("\x84\x58", "johab");
}


void test_find_chars_sjis()
{
  // XXX:
}


void test_find_chars_uhc()
{
  // XXX:
}


PQXX_REGISTER_TEST(test_find_chars_ascii);
PQXX_REGISTER_TEST(test_find_chars_big5);
PQXX_REGISTER_TEST(test_find_chars_gb18030);
PQXX_REGISTER_TEST(test_find_chars_gbk);
PQXX_REGISTER_TEST(test_find_chars_johab);
PQXX_REGISTER_TEST(test_find_chars_sjis);
PQXX_REGISTER_TEST(test_find_chars_uhc);
} // namespace
