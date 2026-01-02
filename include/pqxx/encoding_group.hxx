/** Enum type for supporting encodings in libpqxx
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#if !defined(PQXX_H_ENCODING_GROUP)
#  define PQXX_H_ENCODING_GROUP

#  include <cstddef>

#  include "pqxx/types.hxx"

namespace pqxx
{
/** The supported types of text encoding supported.
 *
 * See:
 * https://www.postgresql.org/docs/current/static/multibyte.html#CHARSET-TABLE
 *
 * This enum does not name the individual supported encodings, only the various
 * schemes for determining where in memory a character ends and a potential new
 * one may begin.  This is crucial for determining such things as where a
 * string ends: a byte in the text may look like an ASCII quote character, but
 * is it really the closing quote, or is it merely a byte inside a multibyte
 * character which just happens to have the same value as the ASCII value for
 * a quote?  This is not an issue in most encodings, but it can happen in
 * others, and can pose a real security risk.
 *
 * Some functions in libpqxx need to know the type of encoding used in a given
 * text in order to find closing quotes or field boundaries.
 *
 * All supported encodings are supersets of ASCII: any byte with a value
 * between 0 and 127 inclusive at the beginning of a character is always a
 * simple, single-byte ASCII character.
 */
enum class encoding_group
{
  /// Default: indeterminate encoding.  All we know is it supports ASCII.
  /** This is the minimum assumption, and therefore the default.
   *
   * We can parse simple SQL values such as integers without knowing more about
   * their encoding, since all supported encodings are supersets of ASCII.  An
   * integer string consists of only digits and an optional sign, so we can
   * parse that without knowing the encoding.
   *
   * But, for example, a quoted string is harder because we can't know _a
   * priori_ whether a byte that looks like the closing quote is indeed an
   * ASCII quote character, or just a trail byte in a multibyte character which
   * just happens to have the same numeric value.
   *
   * Conversions that _may_ run into this problem will throw an exception if
   * the uncoding is `unknown`, even if the actual uncertainty about the
   * meaning of a byte never occurs.  That may seem a little pedantic, but it's
   * better to go through the pain in testing than to risk missing the problem
   * until your code goes into production.
   */
  unknown,

  /// "ASCII-safe" encodings.
  /**
   * This includes all single-byte encodings (such as ASCII or ISO 8859-15)
   * but also all other encodings where there is no risk of ever confusing a
   * byte inside a multibyte character for an ASCII character.  UTF-8 is an
   * example of an ASCII-safe encoding, as are the EUC encodings.
   *
   * This property makes encodings very efficient to parse: to find the next
   * character (if any), just move to the next byte.
   */
  ascii_safe,

  /// Low byte is ASCII, high byte starts a 2-byte character.
  /** Both BIG5 and UHC work like this.  The details vary, but we don't need to
   * validate the input in detail; we just need to be sure that we don't
   * mistake a byte in a multibyte character for a separate special ASCII
   * character (or vice versa if the input ends in mid-character).
   *
   * UHC is, for our purposes, ASCII-safe so long as none of the characters
   * you're looking for are ASCII letters.  So in that common case, feel free
   * to use the `ascii_safe` glyph scanner instead.
   */
  two_tier,

  /// Non-ASCII-safe: GB18030 for Chinese (Traditional & Simplified).
  /** This also covers older subsets such as GBK.
   */
  gb18030,

  /// Non-ASCII-safe: Japanese JIS and Shift JIS.
  sjis,
};
} // namespace pqxx


namespace pqxx::internal
{
/// Function type: "find first occurrence of any of these ASCII characters."
/** This type of function takes a text buffer, and a location in that buffer;
 * it returns the location of the first occurrence within that string, from
 * the `start` position onwards, of any of a specific set of ASCII characters.
 *
 * The `start` offset marks the beginning of the current glyph.  So, if this
 * glyph matches, the function will return `start`
 *
 * If there is no match, returns the end of `haystack`.
 */
using char_finder_func =
  std::size_t(std::string_view haystack, std::size_t start, sl);
} // namespace pqxx::internal

#endif
