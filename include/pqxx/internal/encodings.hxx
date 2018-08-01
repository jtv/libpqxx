/** Interna string encodings support for libpqxx
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_ENCODINGS
#define PQXX_H_ENCODINGS

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <string>


namespace pqxx
{
namespace internal
{

// Encodings supported by PostgreSQL, see
// https://www.postgresql.org/docs/current/static/multibyte.html#CHARSET-TABLE
enum class encoding
{
  BIG5,
  EUC_CN,
  EUC_JP,
  EUC_JIS_2004,
  EUC_KR,
  EUC_TW,
  GB18030,
  GBK,
  ISO_8859_5,
  ISO_8859_6,
  ISO_8859_7,
  ISO_8859_8,
  JOHAB,
  KOI8R,
  KOI8U,
  LATIN1,
  LATIN2,
  LATIN3,
  LATIN4,
  LATIN5,
  LATIN6,
  LATIN7,
  LATIN8,
  LATIN9,
  LATIN10,
  // This encoding is not currently supported by libpqxx, see
  // https://github.com/jtv/libpqxx/issues/97#issuecomment-406107096
  // MULE_INTERNAL,
  SJIS,
  SHIFT_JIS_2004,
  SQL_ASCII,
  UHC,
  UTF8,
  WIN866,
  WIN874,
  WIN1250,
  WIN1251,
  WIN1252,
  WIN1253,
  WIN1254,
  WIN1255,
  WIN1256,
  WIN1257,
  WIN1258
};

/** Struct for representing position & size of multi-byte sequences
 *
 * begin_byte - First byte in the sequence
 * end_byte   - One more than last byte in the sequence
 */
struct seq_position
{
    std::string::size_type begin_byte;
    std::string::size_type   end_byte;
};

/** Get the position & size of the next sequence representing a single glyph
 *
 * The begin_byte will be std::string::npos if there are no more glyphs to
 * extract from the buffer.  For single-byte encodings such as ASCII,
 * (end_byte - begin_byte) will always equal 1 unless begin_byte is npos.
 *
 * The reason the signature is not simpler is so that more informative error
 * messages can be generated.
 *
 * enc        - The character encoding used by the string in the buffer
 * buffer     - The buffer to read from
 * buffer_len - Total size of the buffer
 * start      - Offset in the buffer to start looking for a sequence
 *
 * Throws std::runtime_error for encoding errors (invalid/truncated sequence)
 */
seq_position next_seq(
  encoding enc,
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
);

} // namespace pqxx::internal
} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif
