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

  // Handles all single-byte fixed-width encodings, as well as all other
  // "ASCII-safe" ones.
  //
  // The ASCII-safe encodings are the ones where no byte in a multibyte
  // character can ever have the same value as any ASCII character.
  //
  // UTF-8 is an example of an ASCII-safe encoding: if you're looking for a
  // specific ASCII character in a UTF-8 string, you can just go through the
  // string byte for byte and look for a match.  No extra cleverness required.
  //
  // Cleverness is inefficiency.
  monobyte,

  // Non-ASCII-safe multibyte encodings.
  // These can embed ASCII-like bytes inside multibyte characters,  That's why
  // we need specific support for them.
  big5,
  gb18030,
  gbk,
  johab,
  sjis,
  uhc,
};
} // namespace pqxx


namespace pqxx::internal
{
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
