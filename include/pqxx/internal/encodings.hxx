/** Internal string encodings support for libpqxx
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ENCODINGS
#define PQXX_H_ENCODINGS

#include "pqxx/internal/encoding_group.hxx"

#include <string>
#include <string_view>


namespace pqxx::internal
{
char const *name_encoding(int encoding_id);

/// Convert libpq encoding enum or encoding name to its libpqxx group.
encoding_group enc_group(int /* libpq encoding ID */);
encoding_group enc_group(std::string_view);


/// Look up the glyph scanner function for a given encoding group.
/** To identify the glyph boundaries in a buffer, call this to obtain the
 * scanner function appropriate for the buffer's encoding.  Then, repeatedly
 * call the scanner function to find the glyphs.
 */
PQXX_LIBEXPORT glyph_scanner_func *get_glyph_scanner(encoding_group);


// TODO: Cut corners for ASCII needle.  The "ASCII supersets" act like ASCII.
// TODO: Actually, are we using these at all!?
/// Find a single-byte "needle" character in a "haystack" text buffer.
std::size_t find_with_encoding(
  encoding_group enc, std::string_view haystack, char needle,
  std::size_t start = 0);


PQXX_LIBEXPORT std::size_t find_with_encoding(
  encoding_group enc, std::string_view haystack, std::string_view needle,
  std::size_t start = 0);


/// Iterate over the glyphs in a buffer.
/** Scans the glyphs in the buffer, and for each, passes its begin and its
 * one-past-end pointers to @c callback.
 */
template<typename CALLABLE>
inline void for_glyphs(
  encoding_group enc, CALLABLE callback, char const buffer[],
  std::size_t buffer_len, std::size_t start = 0)
{
  auto const scan{get_glyph_scanner(enc)};
  for (std::size_t here = start, next; here < buffer_len; here = next)
  {
    next = scan(buffer, buffer_len, here);
    callback(buffer + here, buffer + next);
  }
}
} // namespace pqxx::internal
#endif
