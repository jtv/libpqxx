/** Enum type for supporting encodings in libpqxx
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ENCODING_GROUP
#define PQXX_H_ENCODING_GROUP

#include <cstddef>

#include "pqxx/types.hxx"

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
  /// Indeterminate encoding.  There's not much we can do to parse it.
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
  monobyte,

  /// Non-ASCII-safe: Big5 for Traditional Chinese.
  big5,
  /// Non-ASCII-safe: GB18030 for Chinese (Traditional & Simplified).
  gb18030,
  /// Non-ASCII-safe: GuoBiao for Chinese (Traditional & Simplified).
  gbk,
  /// Non-ASCII-safe: JOHAB for Korean.
  johab,
  /// Non-ASCII-safe: Japanese JIS and Shift JIS.
  sjis,
  /// Non-ASCII-safe: Korean Unified Hangul Code.
  uhc,
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
