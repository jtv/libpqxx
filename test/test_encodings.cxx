/** In this test module we'll be searching for the ASCII character '|' in
 * strings in various encodings.
 *
 * This gets interesting in the one scenario where we actually need to know
 * about the text's encoding: if a multibyte character contains a byte whose
 * numerical value happens to be the same as that of the ASCII character we're
 * trying to find.
 *
 * The '|' (pipe) character was chosen because it _can_ occur as a trail byte
 * in all of the supported non-ASCII-safe encodings, with the exception of
 * UHC, where letters are the only ASCII trail bytes allowed.  We don't permit
 * searching for ASCII letters for exactly that reason: with a guarantee that
 * we won't be searching for ASCII letters, it's safe to treat UHC as if it
 * were ASCII-safe as well.
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
auto const eels<pqxx::encoding_group::two_tier>{
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
/// SJIS: Japanese.
template<>
auto const eels<pqxx::encoding_group::sjis>{
  "\x8e\x84\x82\xcc\x83\x7a\x83\x6f\x81\x5b\x83\x4e\x83\x89\x83\x74\x83\x67"
  "\x82\xcd\x83\x45\x83\x69\x83\x4d\x82\xc5\x82\xa2\x82\xc1\x82\xcf\x82\xa2"
  "\x82\xc5\x82\xb7"sv};


/// A tricky test text.
/** These represent multibyte characters in various encodings which happen to
 * contain a byte with the same numeric value as the ASCII pipe symbol, '|'.
 */
template<pqxx::encoding_group ENC> std::string_view const tricky;

template<> auto const tricky<pqxx::encoding_group::two_tier>{"\xa1|"sv};
// (Yeah such a string is not possible here.)
template<> auto const tricky<pqxx::encoding_group::gb18030>{"\x81|"sv};
template<> auto const tricky<pqxx::encoding_group::sjis>{"\x81|"sv};


/// Test basic sanity of search in encoding group `ENC`.
/** Searches test texts for first occurrence of '.' (a dot).
 *
 * Ensure that the text does not contain the ASCII _character_ '.'.  However,
 * it may contain the _byte value_ for that character.  In fact it makes the
 * test stronger and more useful.
 *
 * The texts are both `eels` and `tricky` for each of the encodings.
 */
template<pqxx::encoding_group ENC> void test_search(std::string_view enc_name)
{
  auto const finder{pqxx::internal::get_char_finder<'|'>(ENC, loc())};

  // First, we do some generic tests on ASCII strings.  All supported
  // encodings are ASCII supersets, so a plain ASCII string is valid and
  // correct in each of them.
  PQXX_CHECK_EQUAL(
    finder("", 0, loc()), 0u,
    std::format("Empty string search ({}) went out of bounds.", enc_name));
  PQXX_CHECK_EQUAL(
    finder("|||", 0, loc()), 0u,
    std::format(
      "Search on ASCII string ({}) missed starting char.", enc_name));
  PQXX_CHECK_EQUAL(
    finder("|||", 1, loc()), 1u,
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
    finder(std::string{eels<ENC>} + "|nn", 0, loc()), std::size(eels<ENC>),
    "False negative on search.");

  // Finally, we perform similar searches but for the tricky strings which
  // contain a byte with value 0x7c ("|") inside a multibyte character.  The
  // search should ignore that embedded byte.
  PQXX_CHECK_EQUAL(
    finder(tricky<ENC>, 0, loc()), std::size(tricky<ENC>),
    "Looks like we fell for an embedded '|' byte.");
  // Then, a successful search.
  PQXX_CHECK_EQUAL(
    finder(std::string{tricky<ENC>} + "|nn", 0, loc()), std::size(tricky<ENC>),
    "Did not find '|' after string with embedded '|' byte.");
}


void test_find_chars(pqxx::test::context &)
{
  test_search<pqxx::encoding_group::two_tier>("big5");
  test_search<pqxx::encoding_group::ascii_safe>("ascii_safe");
  test_search<pqxx::encoding_group::gb18030>("gb18030");
  test_search<pqxx::encoding_group::sjis>("sjis");
}


template<pqxx::encoding_group ENC> void check_unfinished_character()
{
  auto const finder{pqxx::internal::get_char_finder<'|'>(ENC, loc())};

  // This happens to be an incomplete character in all supported non-ASCII-safe
  // encodings.
  PQXX_CHECK_THROWS(finder("\x81", 0, loc()), pqxx::argument_error);
}


void test_find_chars_fails_for_unfinished_character(pqxx::test::context &)
{
  check_unfinished_character<pqxx::encoding_group::two_tier>();
  check_unfinished_character<pqxx::encoding_group::gb18030>();
  check_unfinished_character<pqxx::encoding_group::sjis>();
}


template<std::size_t N>
auto find_x(std::array<char, N> const &data, pqxx::encoding_group enc)
{
  auto const find{pqxx::internal::get_char_finder<'|'>(enc, loc())};
  std::string_view const buf{std::data(data), N};
  return find(buf, 0u, loc());
}


void test_find_chars_reports_malencoded_text(pqxx::test::context &tctx)
{
  // Set up an array containing random char values, but not '|'.
  //
  // We really need an amazingly large array here, since our encoding support
  // is only designed to detect structural problems, not invalid characters per
  // se.  So even an array of 500 bytes will pass the SJIS checks far too
  // often.
  std::array<char, 1000> data{};
  for (std::size_t i{0}; i < std::size(data); ++i)
  {
    data.at(i) = tctx.random_char();
    while (data.at(i) == '|') data.at(i) = tctx.random_char();
  }

  // Bet that the random data isn't going to be fully valid text in these
  // encodings.  (Not testing the "two-tier" encodings here, since the only way
  // to get those wrong is in the final byte.)
  PQXX_CHECK_THROWS(
    find_x(data, pqxx::encoding_group::gb18030), pqxx::argument_error);
  PQXX_CHECK_THROWS(
    find_x(data, pqxx::encoding_group::sjis), pqxx::argument_error);
}


PQXX_REGISTER_TEST(test_find_chars);
PQXX_REGISTER_TEST(test_find_chars_fails_for_unfinished_character);
PQXX_REGISTER_TEST(test_find_chars_reports_malencoded_text);
} // namespace
