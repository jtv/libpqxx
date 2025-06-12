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
// Types of encodings supported by PostgreSQL, see
// https://www.postgresql.org/docs/current/static/multibyte.html#CHARSET-TABLE
enum class encoding_group
{
  // Indeterminate encoding.
  unknown,

  // Handles all single-byte fixed-width encodings
  monobyte,

  // XXX: Treat all ASCII-safe ones as monobyte.
  // Multibyte encodings.
  // Some of these can embed ASCII-like bytes inside multibyte characters,
  // notably Big5, SJIS, SHIFT_JIS_2004, GP18030, GBK, JOHAB, UHC.
  big5,
  euc_cn,
  euc_jp,
  euc_kr,
  euc_tw,
  gb18030,
  gbk,
  johab,
  mule_internal,
  sjis,
  uhc,
  utf8,
};
} // namespace pqxx


namespace pqxx::internal
{
// TODO: Get rid of glyph_scanner_func.  Specialise at higher loop level.
/// Function type: "find the end of the current glyph."
/** This type of function takes a text buffer, and a location in that buffer,
 * and returns the location one byte past the end of the current glyph.
 *
 * The start offset marks the beginning of the current glyph.  It must fall
 * within the buffer.
 *
 * There are multiple different glyph scanner implementations, for different
 * kinds of encodings.
 */
using glyph_scanner_func =
  std::size_t(std::string_view buffer, std::size_t start, sl);


/// Function type: "find first occurrence of any of these ASCII characters."
/** This type of function takes a text buffer, and a location in that buffer;
 * it returns the location of the first occurrence within that string of any
 * of a specific set of ASCII characters.
 *
 * For efficiency, it's up to the function to know which those special ASCII
 * characters are.
 *
 * The start offset marks the beginning of the current glyph.
 *
 * Returns the offset of the first matching character, or if there is none, the
 * end of `haystack`.
 */
using char_finder_func =
  std::size_t(std::string_view haystack, std::size_t start, sl);
} // namespace pqxx::internal

#endif
