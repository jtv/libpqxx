/** Internal string encodings support for libpqxx
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
  UTF8
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

encoding_group enc_group(int /* libpq encoding */);
encoding_group enc_group(const std::string&);

/** Get the position & size of the next sequence representing a single glyph
 *
 * The begin_byte will be std::string::npos if there are no more glyphs to
 * extract from the buffer.  For single-byte encodings such as ASCII,
 * (end_byte - begin_byte) will always equal 1 unless begin_byte is npos.
 *
 * The reason the signature is not simpler is so that more informative error
 * messages can be generated.
 *
 * Both runtime and templated versions are available.
 *
 * enc/E      - The character encoding group used by the string in the buffer
 * buffer     - The buffer to read from
 * buffer_len - Total size of the buffer
 * start      - Offset in the buffer to start looking for a sequence
 *
 * Throws std::runtime_error for encoding errors (invalid/truncated sequence)
 */
seq_position next_seq(
  encoding_group enc,
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
);
template<encoding_group E> seq_position next_seq(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
);

// Template specializations for next_seq<>()
#define PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( ENC_GROUP ) \
template<> seq_position next_seq<encoding_group::ENC_GROUP>( \
  const char* buffer, \
  std::string::size_type buffer_len, \
  std::string::size_type start \
);

PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( MONOBYTE )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( BIG5 )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( EUC_CN )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( EUC_KR )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( EUC_TW )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( GB18030 )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( GBK )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( JOHAB )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( MULE_INTERNAL )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( UHC )
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION( UTF8 )

#undef PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION

} // namespace pqxx::internal
} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif
