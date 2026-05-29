#ifndef PQXX_INTERNAL_STRINGS_HXX
#define PQXX_INTERNAL_STRINGS_HXX

/* Helpers for dealing with SQL strings.
 */

#include <cassert>

#include "pqxx/internal/encodings.hxx"
#include "pqxx/util.hxx"


namespace pqxx::internal
{
/// The width in bytes of a single ASCII character.  In other words, one.
constexpr std::size_t one_ascii_char{1u};


/// Does `HAYSTACK...` contain `needle`?
template<auto... HAYSTACK>
[[nodiscard]] inline constexpr bool pack_contains(auto const &needle) noexcept
{
  return ((HAYSTACK == needle) or ...);
}


/// Find the next double-quote or backslash.
/** `ESC` is either one or two ASCII characters: double-quote (`"`) and/or
 * backslash (`\\`).
 */
template<encoding_group ENC, char... ESC>
[[nodiscard]] inline constexpr std::size_t
find_dquote_or_backslash(std::string_view input, std::size_t pos, sl loc)
{
  static_assert(sizeof...(ESC) >= 1);
  static_assert(sizeof...(ESC) <= 2);
  static_assert((((ESC == '\\') or (ESC == '"')) and ...));

  // Search for a double-quote character, or a backslash _if there is a
  // backslash in ESC._
  //
  // It would be so, so nice to be able to pass both ESC and '"' as template
  // arguments here..  The language is fine with it, but some tools and
  // compilers warn when they see a redundant condition.
  if constexpr (pack_contains<ESC...>('"'))
    return find_ascii_char<ENC, ESC...>(input, pos, loc);
  else
    return find_ascii_char<ENC, ESC..., '"'>(input, pos, loc);
}


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
    pos = find_dquote_or_backslash<ENC, ESC...>(input, pos, loc);

    if (pos == sz)
      throw argument_error{"Unterminated string.", loc};
    char const found{input[pos]};

    assert((found == '"') or pack_contains<ESC...>(found));

    // Consume the character.
    pos += one_ascii_char;

    switch (found)
    {
    case '"':
      if (pack_contains<ESC...>('"') and (pos < sz) and (input[pos] == '"'))
        // Doubled-double-quote escape.
        pos += one_ascii_char;
      else
        // Closing quote.
        return pos;
      break;

    case '\\':
      // Backslash escape.
      // We won't get here unless the backslash acts as an escape character.
      assert(pack_contains<ESC...>('\\'));
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


/// Un-quote and un-escape a double-quoted SQL string into an output buffer.
/** `ESC` are the escape characters.  I've found 3 kinds of double-quoted
 * strings in various SQL situations: ones that use the backslash as an escape
 * character, ones that escape a nested quote by repeating it, and ones that
 * accept both.  So for `ESC` fill in a double-quote, a backslash, or both.
 *
 * @param output Buffer for results.
 * @param input Text.  The double-quoted string must start at offset `pos`,
 * and must end at the end of `input`.
 * @return Number of bytes written to `output`.
 */
template<encoding_group ENC, char... ESC>
PQXX_INLINE_COV inline constexpr std::size_t parse_double_quoted_string(
  std::span<char> output, std::string_view input, std::size_t pos, sl loc)
{
  static_assert(sizeof...(ESC) >= 1);
  static_assert(sizeof...(ESC) <= 2);
  static_assert((((ESC == '\\') or (ESC == '"')) and ...));

  auto const end{std::size(input)};
  assert((end - pos) > 1);
  assert(pos < end);

  auto const closing_quote{end - 1};
  assert(closing_quote < end);
  assert(input[closing_quote] == '"');

  // We're at the starting quote.  Skip it.
  assert(pos < closing_quote);
  assert(input[pos] == '"');

  // How much room do we have?
  auto const budget{std::size(output)};
  if (std::size(input) > (budget + 2))
    throw conversion_overrun{
      "Not enough buffer space to parse double-quoted string.", loc};

  // Writing position.
  std::size_t write{0u};

  pos += one_ascii_char;
  assert(pos <= closing_quote);

  // In theory, our knowledge of the closing quote's offset should mean that
  // there's no need for the find_ascii_char() call to check for end-of-string
  // inside its loop.  Not sure whether the compiler will be smart enough to
  // see that though.

  while (pos < closing_quote)
  {
    // XXX: Actually we don't need to look for '"' if it's not an escape!
    // Race straight to the first special character.  If we're in a context
    // where the backslash acts as an escape character, look for that.  Look
    // for a double-quote character regardless.
    std::size_t const special{
      find_dquote_or_backslash<ENC, ESC...>(input, pos, loc)};
    assert(special <= closing_quote);

    // The stretch that we've just skipped over is plain vanilla text, no
    // nasty escapes.  Copy it unchanged.
    write =
      copy_chars<false>(input.substr(pos, special - pos), output, write, loc);
    assert(write < budget);
    pos = special;
    char const found{input[pos]};

    // We're at either the closing quote or an escape character.
    assert((found == '"') or pack_contains<ESC...>(found));
    // TODO: Isn't this the only way out of this loop?  Try to restructure.
    if (pos == closing_quote)
      return write;

    // We're at an escape character.
    assert(pack_contains<ESC...>(found));
    pos += one_ascii_char;

    // We're at the escaped character.
    //
    // If the input has been scanned correctly, the string can't end here.
    assert(pos < closing_quote);
    if ((input[pos] == '"') or pack_contains<ESC...>(input[pos]))
    {
      // We know that the escaped character is a single-byte one.  And it's one
      // we have to consume right now; if we left it to the next iteration it
      // would confuse the next find_ascii_char() call.
      assert(write < budget);
      output[write] = input[pos];
      write += one_ascii_char;
      ;
      pos += one_ascii_char;
    }
    else
    {
      // This could be a multibyte character.  But no matter: it's not special
      // so we can let the next iteration handle it.
    }
  }
  assert(pos == closing_quote);
  assert(write <= end);
  return write;
}


/// Un-quote and un-escape a double-quoted SQL string.
/** `ESC` are the escape characters.  I've found 3 kinds of double-quoted
 * strings in various SQL situations: ones that use the backslash as an escape
 * character, ones that escape a nested quote by repeating it, and ones that
 * accept both.  So for `ESC` fill in a double-quote, a backslash, or both.
 *
 * @param input Text.  The double-quoted string must start at offset `pos`,
 * and must end at the end of `input`.  So, truncate `input` before calling if
 * necessary.
 * @return Parsed string.
 */
template<encoding_group ENC, char... ESC>
PQXX_INLINE_COV inline constexpr std::string
parse_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  std::string output;
  // Unescaping can shrink a string, but never grow it, so this is enough
  // room for the output.
  output.resize(std::size(input));
  output.resize(
    parse_double_quoted_string<ENC, ESC...>(output, input, pos, loc));
  return output;
}


/// Find the end of an unquoted string in an array or composite-type value.
/** Stops when it gets to the end of the input; or when it sees any of the
 * characters in STOP which has not been escaped.
 *
 * For array values, STOP is an array element separator (typically comma, or
 * semicolon), or a closing brace.  For a value of a composite type, STOP is a
 * comma or a closing parenthesis.
 */
template<encoding_group ENC, char... STOP>
PQXX_INLINE_COV inline constexpr std::size_t
scan_unquoted_string(std::string_view input, std::size_t pos, sl loc)
{
  // Backslash is the escape character.  It can't also be a string terminator.
  assert(not((STOP == '\\') or ...));
  auto const sz{std::size(input)};

  while (pos < sz)
  {
    pos = find_ascii_char<ENC, '\\', STOP...>(input, pos, loc);
    if ((pos < sz) and (input.at(pos) == '\\'))
    {
      pos += one_ascii_char;
      if (pos == sz)
        throw argument_error{
          "Unquoted string unexpectedly ended in backslash.", loc};
    }
    else
    {
      // We hit the end of the string.
      assert(((input.at(pos) == STOP) or ...));
      return pos;
    }
  }

  // We hit the end of our input.  The string must end here.
  return pos;
}


/// Parse an unquoted array entry or field of a composite-type value.
/** @param input A view on the text, truncated at the end of the string.  So,
 *     the end of `input` must coincide with the end of the string.  Truncate
 *     before calling if necessary.
 * @param pos The string's starting offset within `input`.
 */
template<encoding_group ENC>
PQXX_INLINE_ONLY inline constexpr std::size_t parse_unquoted_string(
  std::span<char> output, std::string_view input, std::size_t pos, sl loc)
{
  auto const sz{std::size(input)};
  std::size_t write{0u};

  // Unquoted strings always ignore leading & trailing whitespace.  Even when
  // whitespace would normally terminate the string.
  pos = skip_ascii_whitespace(input, pos);

  while (pos < sz)
  {
    auto const next{find_ascii_char<ENC, '\\'>(input, pos, loc)};
    write =
      copy_chars<false>(input.substr(pos, next - pos), output, write, loc);
    pos = next;
    if ((pos < sz) and (input.at(pos) == '\\'))
    {
      pos += one_ascii_char;
      assert(pos < sz);
      if (static_cast<unsigned char>(input.at(pos)) < 127)
      {
        // The good news: the escaped character is an ASCII character and
        // therefore single-byte.
        //
        // The bad news: we can't leave it for the next iteration to handle,
        // because it may be another backslash.
        output[write] = input.at(pos);
        write += one_ascii_char;
      }
      else
      {
        // The bad news: we may be at the beginning of a multibyte character.
        //
        // The good news: it's not special to the parser, so we can just leave
        // it for the next iteration to copy into the output.  This saves us
        // having to find the next character boundary.  Pretend that the
        // character wasn't escaped at all.
      }
    }
  }

  assert(write <= sz);
  // XXX: Skip trailing whitespace...  How do we do that efficiently?
  return write;
}


/// Parse an unquoted array entry or field of a composite-type value.
/** @param output Buffer for results.
 * @param input A view on the text, truncated at the end of the string.  So,
 *     the end of `input` must coincide with the end of the string.  Truncate
 *     before calling if necessary.
 * @param pos The string's starting offset within `input`.
 */
template<encoding_group ENC>
PQXX_INLINE_ONLY inline constexpr std::string
parse_unquoted_string(std::string_view input, std::size_t pos, sl loc)
{
  std::string output;
  output.resize(std::size(input));
  output.resize(parse_unquoted_string<ENC>(output, input, pos, loc));
  return output;
}

} // namespace pqxx::internal
#endif
