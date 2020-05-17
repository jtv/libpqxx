/** Enum type for supporting encodings in libpqxx
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ENCODING_GROUP
#define PQXX_H_ENCODING_GROUP

#include <cstddef>

namespace pqxx::internal
{
// Types of encodings supported by PostgreSQL, see
// https://www.postgresql.org/docs/current/static/multibyte.html#CHARSET-TABLE
enum class encoding_group
{
  // Handles all single-byte fixed-width encodings
  MONOBYTE,

  // Multibyte encodings
  BIG5,
  EUC_CN,
  EUC_JP,
  EUC_JIS_2004,
  EUC_KR,
  EUC_TW,
  GB18030,
  GBK,
  JOHAB,
  MULE_INTERNAL,
  SJIS,
  SHIFT_JIS_2004,
  UHC,
  UTF8,
};


/// Function type: "find the end of the current glyph."
/** This type of function takes a text buffer, and a location in that buffer,
 * and returns the location one byte past the end of the current glyph.
 *
 * The start offset marks the beginning of the current glyph.  It must fall
 * within the buffer.
 *
 * There are multiple different glyph scnaner implementations, for different
 * kinds of encodings.
 */
using glyph_scanner_func =
  std::size_t(char const buffer[], std::size_t buffer_len, std::size_t start);
} // namespace pqxx::internal

#endif
