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
#include "pqxx/internal/encoding_group.hxx"

#include <string>


namespace pqxx
{
namespace internal
{

/// Reference to a glyph (possibly multi-byte) in a string.
struct seq_position
{
  /// Offset within the string of glyph's starting byte.
  std::string::size_type begin_byte;
  /// One byte past the offset within the string of the glyph's final byte.
  std::string::size_type end_byte;
};


/// Convert libpq encoding enum or encoding name to its libpqxx group.
encoding_group enc_group(int /* libpq encoding ID */);
encoding_group enc_group(const std::string&);


/** Get the position & size of the next sequence representing a single glyph.
 *
 * Statically specialised for a single encoding.  There is also a dynamic,
 * non-templated version which does a bit more work at runtime.
 *
 * The begin_byte will be std::string::npos if there are no more glyphs to
 * extract from the buffer.  For single-byte encodings such as ASCII,
 * (end_byte - begin_byte) will always equal 1 unless begin_byte is npos.
 *
 * Some arguments serve only to generate more helpful error messages.
 *
 * enc/E      - The character encoding group used by the string in the buffer
 * buffer     - The buffer to read from
 * buffer_len - Total size of the buffer
 * start      - Offset in the buffer to start looking for a sequence
 *
 * Throws std::runtime_error for encoding errors (invalid/truncated sequence)
 */
template<encoding_group E> seq_position next_seq(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
);
seq_position next_seq(
  encoding_group enc,
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
);

// Template specializations for next_seq<>()
#define PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(ENC_GROUP) \
template<> seq_position next_seq<encoding_group::ENC_GROUP>( \
  const char* buffer, \
  std::string::size_type buffer_len, \
  std::string::size_type start \
);

PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(MONOBYTE)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(BIG5)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(EUC_CN)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(EUC_KR)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(EUC_TW)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(GB18030)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(GBK)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(JOHAB)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(MULE_INTERNAL)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(UHC)
PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION(UTF8)

#undef PQXX_INTERNAL_DECLARE_NEXT_SEQ_SPECIALIZATION

template<encoding_group E> std::string::size_type find_with_encoding(
  const std::string& haystack,
  char needle,
  std::string::size_type start = 0
)
{
  while (start < haystack.size())
  {
    auto here = next_seq<E>(
      haystack.c_str(),
      haystack.size(),
      start
    );
    if (here.begin_byte == std::string::npos)
      break;
    else if (
      here.end_byte - here.begin_byte == 1
      && haystack[here.begin_byte] == needle
    )
      return here.begin_byte;
    else
      start = here.end_byte;
  }
  return std::string::npos;
}
std::string::size_type find_with_encoding(
  encoding_group enc,
  const std::string& haystack,
  char needle,
  std::string::size_type start = 0
);

template<encoding_group E> std::string::size_type find_with_encoding(
  const std::string& haystack,
  const std::string& needle,
  std::string::size_type start = 0
)
{
  while (start < haystack.size())
  {
    auto here = next_seq<E>(
      haystack.c_str(),
      haystack.size(),
      start
    );
    if (here.begin_byte == std::string::npos)
      break;
    else
    {
      const char* ch{haystack.c_str() + start};
      for (auto cn : needle)
        if (*ch != cn) goto next;
        else ++ch;
      return here.begin_byte;
    }
  next:
    start = here.end_byte;
  }
  return std::string::npos;
}
std::string::size_type find_with_encoding(
  encoding_group enc,
  const std::string& haystack,
  const std::string& needle,
  std::string::size_type start = 0
);

} // namespace pqxx::internal
} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"
#endif
