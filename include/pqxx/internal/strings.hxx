#ifndef PQXX_INTERNAL_STRINGS_HXX
#define PQXX_INTERNAL_STRINGS_HXX

/* Helpers for dealing with SQL strings.
 */

#include <cassert>

#include "pqxx/internal/encodings.hxx"
#include "pqxx/util.hxx"


namespace pqxx::internal
{
// The width in bytes of a single ASCII character.  In other words, one.
constexpr std::size_t one_ascii_char{1u};


// Find the end of a double-quoted SQL string.
/** `input[pos]` must be the opening double quote.
 *
 * The backend double-quotes strings in composites or arrays, when needed, as
 * well as in hstore values.  Special characters are escaped using backslashes.
 *
 * `ESC` are the escape characters.  I've found 3 kinds of double-quoted
 * strings in various SQL situations: ones that use the backslash as an escape
 * character, ones that escape a nested quote by repeating it, and ones that
 * accept both.  So for `ESC` fill in a double-quote, a backslash, or both.
 *
 * Returns the offset of the first position after the closing quote.
 */
template<encoding_group ENC, char... ESC>
PQXX_INLINE_COV inline constexpr std::size_t
scan_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  static_assert(sizeof...(ESC) >= 1);
  static_assert(sizeof...(ESC) <= 2);
  static_assert((((ESC == '\\') or (ESC == '"')) and ...));

  assert(input[pos] == '"');
  auto const sz{std::size(input)};

  // Skip over the opening double-quote.
  pos += one_ascii_char;

  do {
    // Find the next "interesting" character: a double-quote or a backslash (if
    // backslashes act as escape characters here).
    pos = find_ascii_char<ENC, ESC..., '"'>(input, pos, loc);
    if (pos == sz)
      throw argument_error{"Unterminated string.", loc};
    char const found{input[pos]};
    // NOLINTNEXTLINE(misc-redundant-expression)
    assert((found == '"') or ((found == ESC) or ...));

    // Consume the character.
    pos += one_ascii_char;

    switch (found)
    {
    case '"':
      if (((ESC == '"') or ...) and (pos < sz) and (input[pos] == '"'))
        // Doubled-double-quote escape.
        pos += one_ascii_char;
      else
        // Closing quote.
        return pos;
      break;

    case '\\':
      // Backslash escape.
      // We won't get here unless the backslash acts as an escape character.
      assert(((ESC == '\\') or ...));
      if (pos == sz)
        throw argument_error{"String unexpectedly ends in backslash.", loc};
      if (static_cast<unsigned char>(input[pos]) < 127)
      {
        // Escaped ASCII character.  Consume it here, in case it's another
        // backslash or an escaped double-quote (which would confuse the next
        // iteration otherwise).
        pos += one_ascii_char;
        if (pos == sz)
          throw argument_error{
            "Unexpected end of string: escape sequence.", loc};
      }
      else
      {
        // Leave escaped multibyte character to the next iteration, so that
        // find_ascii_char() will figure out where the character ends.
      }
      break;

    default: PQXX_UNREACHABLE;
    }
  } while (pos < sz);

  // If we got here, we never found the closing double-quote.
  throw argument_error{"Unterminated string: " + std::string{input}, loc};
}


#ifndef NDEBUG
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"'>(
    "\"\"", 0u, sl::current()) == 2);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\'>(
    "\"\"", 0u, sl::current()) == 2);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\', '"'>(
    "\"\"", 0u, sl::current()) == 2);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"', '\\'>(
    "\"\"", 0u, sl::current()) == 2);

static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"'>(
    "\"x\"y", 0u, sl::current()) == 3);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\'>(
    "\"x\"y", 0u, sl::current()) == 3);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\', '"'>(
    "\"x\"y", 0u, sl::current()) == 3);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"', '\\'>(
    "\"x\"y", 0u, sl::current()) == 3);

static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"'>(
    " \"\"", 1u, sl::current()) == 3);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\'>(
    " \"\"", 1u, sl::current()) == 3);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\', '"'>(
    " \"\"", 1u, sl::current()) == 3);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"', '\\'>(
    " \"\"", 1u, sl::current()) == 3);

static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"'>(
    "\"\\\"\"\"\"\"", 0u, sl::current()) == 7);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\'>(
    "\"\\\"\"\"\"", 0u, sl::current()) == 4);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\', '"'>(
    "\"\\\"\"\"\"", 0u, sl::current()) == 6);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"', '\\'>(
    "\"\\\"\"\"\"", 0u, sl::current()) == 6);

static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"'>(
    "\"\\\\\"", 0u, sl::current()) == 4);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\'>(
    "\"\\\\\"", 0u, sl::current()) == 4);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '\\', '"'>(
    "\"\\\\\"", 0u, sl::current()) == 4);
static_assert(
  scan_double_quoted_string<encoding_group::ascii_safe, '"', '\\'>(
    "\"\\\\\"", 0u, sl::current()) == 4);
#endif // NDEBUG


// TODO: Needs version with caller-supplied buffer.
/// Un-quote and un-escape a double-quoted SQL string.
/** `ESC` are the escape characters.  I've found 3 kinds of double-quoted
 * strings in various SQL situations: ones that use the backslash as an escape
 * character, ones that escape a nested quote by repeating it, and ones that
 * accept both.  So for `ESC` fill in a double-quote, a backslash, or both.
 *
 * @param input Text.  The double-quoted string must start at offset `pos`,
 * and must end at the end of `input`.  So, truncate `input` before calling if
 * necessary.
 */
template<encoding_group ENC, char... ESC>
PQXX_INLINE_COV inline constexpr std::string
parse_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  static_assert(sizeof...(ESC) >= 1);
  static_assert(sizeof...(ESC) <= 2);
  static_assert((((ESC == '\\') or (ESC == '"')) and ...));

  std::string output;
  auto const end{std::size(input)};
  assert((end - pos) > 1);

  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  Or in the extreme opposite case, the real number could be
  // half that.  Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - pos - 2));

  auto const closing_quote{end - 1};
  assert(closing_quote < end);
  assert(input[closing_quote] == '"');

  // We're at the starting quote.  Skip it.
  assert(pos < closing_quote);
  assert(input[pos] == '"');
  pos += one_ascii_char;
  assert(pos <= closing_quote);

  // In theory, our knowledge of the closing quote's offset should mean that
  // there's no need for the find_ascii_char() call to check for end-of-string
  // inside its loop.  Not sure whether the compiler will be smart enough to
  // see that though.

  while (pos < closing_quote)
  {
    // Race straight to the first special character.  If we're in a context
    // where the backslash acts as an escape character, look for that.  Look
    // for a double-quote character regardless.
    std::size_t const special{
      find_ascii_char<ENC, ESC..., '"'>(input, pos, loc)};
    assert(special <= closing_quote);

    // The stretch that we've just skipped over is plain vanilla text, no
    // nasty escapes.  Copy it unchanged.
    output.append(input.substr(pos, special - pos));
    pos = special;
    char const found{input[pos]};

    // We're at either the closing quote or an escape character.
    assert((found == '"') or ((found == ESC) or ...));
    // TODO: Isn't this the only way out of this loop?  Try to restructure.
    if (pos == closing_quote)
      return output;

    // We're at an escape character.
    assert(((found == ESC) or ...));
    pos += one_ascii_char;

    // We're at the escaped character.
    //
    // If the input has been scanned correctly, the string can't end here.
    assert(pos < closing_quote);
    if (((input[pos] == ESC) or ...) or (input[pos] == '"'))
    {
      // We know that the escaped character is a single-byte one.  And it's one
      // we have to consume right now; if we left it to the next iteration it
      // would confuse the next find_ascii_char() call.
      output.push_back(input[pos]);
      pos += one_ascii_char;
    }
    else
    {
      // This could be a multibyte character.  But no matter: it's not special
      // wo we can let the next iteration handle it.
    }
  }
  assert(pos == closing_quote);

  return output;
}
} // namespace pqxx::internal
#endif
