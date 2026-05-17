#ifndef PQXX_INTERNAL_ARRAY_COMPOSITE_HXX
#define PQXX_INTERNAL_ARRAY_COMPOSITE_HXX

/* Helpers for dealing with SQL strings.
 */

#include <cassert>

#include "pqxx/internal/encodings.hxx"
#include "pqxx/util.hxx"


namespace pqxx::internal
{
// The width in bytes of a single ASCII character.  In other words, one.
constexpr std::size_t one_ascii_char{1u};


// Find the end of a double-quoted string.
/** `input[pos]` must be the opening double quote.
 *
 * The backend double-quotes strings in composites or arrays, when needed.
 * Special characters are escaped using backslashes.
 *
 * Returns the offset of the first position after the closing quote.
 */
template<encoding_group ENC>
PQXX_INLINE_COV inline constexpr std::size_t
scan_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  assert(input[pos] == '"');
  auto const sz{std::size(input)};

  // Skip over the opening double-quote, and after that, any leading
  // "un-interesting" characters.
  pos = find_ascii_char<ENC, '"', '\\'>(input, pos + one_ascii_char, loc);
  while (pos < sz)
  {
    // No need to check for a multibyte character here: if it's multibyte, its
    // first byte won't match either of these ASCII characters.
    switch (input[pos])
    {
    case '"':
      // Is this the closing quote we're looking for?  Scan ahead to find out.
      pos += one_ascii_char;
      if (pos >= sz)
      {
        // Clear-cut case.  This is the closing quote and it's right at the end
        // of the input.
        return pos;
      }
      else if (input[pos] == '"')
      {
        // What we found is a doubled-up double-quote.  That's the other way of
        // escaping them.  Why can't this ever be simple?
        pos += one_ascii_char;
        if (pos >= sz)
          throw argument_error{
            "Unexpected end of string: double double-quote."};
      }
      else
      {
        // This was the closing quote (though not at the end of the input).
        // We are now at the one-past-end position.
        return pos;
      }
      break;

    case '\\':
      // Backslash escape.  Move on to the next character, so that at the end
      // of the iteration we'll skip right over it.
      pos += one_ascii_char;
      if (pos >= sz)
        throw argument_error{"Unexpected end of string: backslash.", loc};

      if ((input[pos] == '\\') or (input[pos] == '"'))
      {
        // As you'd expect: the backslash escapes a double-quote, or another
        // backslash.  Move past it, or the find_ascii_char<>() at the end of
        // the iteration will just stop here again.
        pos += one_ascii_char;
        if (pos >= sz)
          throw argument_error{
            "Unexpected end of string: escape sequence.", loc};
      }
      break;
    }

    // We've reached the end of one iteration without reaching the end of the
    // string.
    pos = find_ascii_char<ENC, '"', '\\'>(input, pos, loc);
  }

  // If we got here, we never found the closing double-quote.
  throw argument_error{
    "Missing closing double-quote: " + std::string{input}, loc};
}


// TODO: Needs version with caller-supplied buffer.
/// Un-quote and un-escape a double-quoted SQL string.
/** @param input Text.  The double-quoted string must start at offset `pos`,
 * and must end at the end of `input`.  So, truncate `input` before calling if
 * necessary.
 */
template<encoding_group ENC>
PQXX_INLINE_COV inline constexpr std::string
parse_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  std::string output;
  auto const end{std::size(input)};
  assert((end - pos) > 1);
  assert(input[end - 1] == '"');

  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  Or in the extreme opposite case, the real number could be
  // half that.  Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - pos - 2));

  auto const closing_quote{end - 1};

  // We're at the starting quote.  Skip it.
  assert(pos < closing_quote);
  assert(input[pos] == '"');
  pos += one_ascii_char;
  assert(pos <= closing_quote);

  // In theory, the closing quote should mean that there's no need for the
  // find_ascii_char() call to check for end-of-string inside its loop.  Not
  // sure whether the compiler will be smart enough to see that though.
  assert(input[closing_quote] == '"');

  while (pos < closing_quote)
  {
    auto const next{find_ascii_char<ENC, '"', '\\'>(input, pos, loc)};
    output.append(input.substr(pos, next - pos));
    pos = next;
    assert(pos <= closing_quote);
    assert((input[pos] == '"') or (input[pos] == '\\'));

    if (pos >= closing_quote)
      return output;

    // We're at either a backslash or a double-quote... and we're not at the
    // closing quote.  Therefore, we're at an escape character.  Skip it.
    pos += one_ascii_char;

    // We are now at the escaped character.
    // If the input has been scanned correctly, the string can't end here.
    assert(pos < closing_quote);

    if ((input[pos] == '"') or (input[pos] == '\\'))
    {
      // We know this is a single-byte character.  Append that (skipping the
      // escaping character) and move on to the next character.
      output.push_back(input[pos]);
      pos += one_ascii_char;
    }
    else
    {
      // This could be a multibyte character.  But no matter: we can let the
      // next iteration handle it like any run-of-the-mill character.
    }
  }
  assert(pos == closing_quote);

  return output;
}
} // namespace pqxx::internal
#endif
