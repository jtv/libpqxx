/** In this test module we'll be searching for the ASCII character 'X' in
 * strings in various encodings.
 *
 * This gets interesting in the one scenario where we actually need to know
 * about the text's encoding: if a multibyte character contains a byte whose
 * numerical value happens to be the same as that of the ASCII character we're
 * trying to find.
 */

#include "helpers.hxx"

#include "pqxx/internal/encodings.hxx"


namespace
{
using namespace std::literals;


/// Convenience shorthand: `std::source_location::current()`.
pqxx::sl loc(pqxx::sl l = pqxx::sl::current())
{
  return l;
}


/// A simple test text, no special tricks.
/** The text is "My hovercraft is full of eels" translated to various
 * languages using Google Translate, and encoded in the respective encoding
 * groups.
 */
template<pqxx::encoding_group ENC> std::string_view const eels;

/// Big5: Traditional Chinese.
template<>
auto const eels<pqxx::encoding_group::big5>{
  "\xa7\xda\xaa\xba\xae\xf0\xb9\xd4\xb2\xee\xb8\xcc\xa5\xfe\xac\x4f\xc5\xc1"
  "\xb3\xbd"sv};
/// ASCII-safe: German.
template<>
auto const eels<pqxx::encoding_group::ascii_safe>{
  "Mein Luftkissenfahrzeug ist voll mit Aalen."sv};
/// GB18030: Simplified Chinese.
template<>
auto const eels<pqxx::encoding_group::gb18030>{
  "\xce\xd2\xb5\xc4\xc6\xf8\xb5\xe6\xb4\xac\xc0\xef\xd7\xb0\xc2\xfa\xc1\xcb"
  "\xf7\xa9\xd3\xe3\xa1\xa3"sv};
/// GBK: Simplified Chinese.
template<>
auto const eels<pqxx::encoding_group::gbk>{
  "\xce\xd2\xb5\xc4\xc6\xf8\xb5\xe6\xb4\xac\xc0\xef\xd7\xb0\xc2\xfa\xc1\xcb"
  "\xf7\xa9\xd3\xe3\xa1\xa3"sv};
/// JOHAB: Korean.
template<>
auto const eels<pqxx::encoding_group::johab>{
  "\x90\x81 \xd1\xa1\xa4\xe1\xc7\x61\x9c\x81\xcf\x61\xcb\x61\x93\x65 "
  "\xb8\x77\xb4\xe1\x9d\xa1 \x88\x61\x97\x62 \xc0\x61 "
  "\xb7\xb6\xb4\xe1\xb6\x61"sv};
/// SJIS: Japanese.
template<>
auto const eels<pqxx::encoding_group::sjis>{
  "\x8e\x84\x82\xcc\x83\x7a\x83\x6f\x81\x5b\x83\x4e\x83\x89\x83\x74\x83\x67"
  "\x82\xcd\x83\x45\x83\x69\x83\x4d\x82\xc5\x82\xa2\x82\xc1\x82\xcf\x82\xa2"
  "\x82\xc5\x82\xb7"sv};
/// UHC: Korean.
template<>
auto const eels<pqxx::encoding_group::uhc>{
  "\xb3\xbb \xc8\xa3\xb9\xf6\xc5\xa9\xb7\xa1\xc7\xc1\xc6\xae\xb4\xc2 "
  "\xc0\xe5\xbe\xee\xb7\xce \xb0\xa1\xb5\xe6 \xc2\xf7 "
  "\xc0\xd6\xbe\xee\xbf\xe4"sv};


/// A tricky test text.
/** These represent multibyte characters in various encodings which happen to
 * contain a byte with the same numeric value as the ASCII letter 'X'.
 */
template<pqxx::encoding_group ENC> std::string_view const tricky;

template<> auto const tricky<pqxx::encoding_group::big5>{"\xaa\x58"sv};
// (Yeah such a string is not possible here.)
template<> auto const tricky<pqxx::encoding_group::ascii_safe>{""sv};
template<> auto const tricky<pqxx::encoding_group::gb18030>{"\x81\x58"sv};
template<> auto const tricky<pqxx::encoding_group::gbk>{"\xaa\x58"sv};
template<> auto const tricky<pqxx::encoding_group::johab>{"\x84\x58"sv};
template<> auto const tricky<pqxx::encoding_group::sjis>{"\x81\x58"sv};
template<> auto const tricky<pqxx::encoding_group::uhc>{"\xa1\x58"sv};


/// Test basic sanity of search in encoding group `ENC`.
/** Searches test texts for first occurrence of 'X'.
 *
 * Ensure that the text does not contain the ASCII _character_ X.  However, it
 * may contain the _byte value_ for that character.  In fact it makes the test
 * stronger and more useful.
 *
 * The texts are both `eels` and `tricky` for each of the encodings.
 */
template<pqxx::encoding_group ENC> void test_search(std::string_view enc_name)
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

  // Now try searching a text that actually uses ENC.  First a failing search,
  // since the text does not contain the character we're looking for:
  PQXX_CHECK_EQUAL(
    finder(eels<ENC>, 0, loc()), std::size(eels<ENC>),
    "Search for absent character did not hit end.");
  // Then, a successful search.
  PQXX_CHECK_EQUAL(
    finder(std::string{eels<ENC>} + "Xnn", 0, loc()), std::size(eels<ENC>),
    "False negative on search.");

  // Finally, we perform similar searches but for the tricky strings which
  // contain a byte with value 'X' inside a multibyte character.  The search
  // should ignore that embedded byte.
  PQXX_CHECK_EQUAL(
    finder(tricky<ENC>, 0, loc()), std::size(tricky<ENC>),
    "Looks like we fell for an embedded 'X' byte.");
  // Then, a successful search.
  PQXX_CHECK_EQUAL(
    finder(std::string{tricky<ENC>} + "Xnn", 0, loc()), std::size(tricky<ENC>),
    "Did not find 'X' after string with embedded 'X' byte.");
}


void test_find_chars()
{
  test_search<pqxx::encoding_group::big5>("big5");
  test_search<pqxx::encoding_group::ascii_safe>("ascii_safe");
  test_search<pqxx::encoding_group::gb18030>("gb18030");
  test_search<pqxx::encoding_group::gbk>("gbk");
  test_search<pqxx::encoding_group::johab>("johab");
  test_search<pqxx::encoding_group::sjis>("sjis");
  test_search<pqxx::encoding_group::uhc>("uhc");
}


template<pqxx::encoding_group ENC> void check_unfinished_character()
{
  auto const finder{pqxx::internal::get_char_finder<'X'>(ENC, loc())};

  // This happens to be an incomplete character in all supported non-ASCII-safe
  // encodings.
  PQXX_CHECK_THROWS(finder("\x81", 0, loc()), pqxx::argument_error);
}


void test_find_chars_fails_for_unfinished_character()
{
  check_unfinished_character<pqxx::encoding_group::big5>();
  check_unfinished_character<pqxx::encoding_group::gb18030>();
  check_unfinished_character<pqxx::encoding_group::gbk>();
  check_unfinished_character<pqxx::encoding_group::johab>();
  check_unfinished_character<pqxx::encoding_group::sjis>();
  check_unfinished_character<pqxx::encoding_group::uhc>();
}


/// Generate a random char value.
inline char random_char()
{
  return static_cast<char>(
    static_cast<std::uint8_t>(pqxx::test::make_num(256)));
}


template<std::size_t N>
auto find_x(std::array<char, N> const &data, pqxx::encoding_group enc)
{
  auto const find{pqxx::internal::get_char_finder<'X'>(enc, loc())};
  std::string_view const buf{std::data(data), N};
  return find(buf, 0u, loc());
}


void test_find_chars_reports_malencoded_text()
{
  // Set up an array containing random char values, but not 'X'.
  std::array<char, 100> data{};
  for (std::size_t i{0}; i < std::size(data); ++i)
  {
    data.at(i) = random_char();
    while (data.at(i) == 'X') data.at(i) = random_char();
  }

  pqxx::encoding_group const unsafe[]{
    pqxx::encoding_group::big5, pqxx::encoding_group::gb18030,
    pqxx::encoding_group::gbk,  pqxx::encoding_group::johab,
    pqxx::encoding_group::sjis, pqxx::encoding_group::uhc,
  };

  // Bet that the random data isn't going to be fully correct in any of the
  // ASCII-unsafe encodings.
  for (auto const enc : unsafe)
    PQXX_CHECK_THROWS(find_x(data, enc), pqxx::argument_error);
}


PQXX_REGISTER_TEST(test_find_chars);
PQXX_REGISTER_TEST(test_find_chars_fails_for_unfinished_character);
PQXX_REGISTER_TEST(test_find_chars_reports_malencoded_text);
} // namespace
