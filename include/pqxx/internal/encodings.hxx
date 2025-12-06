/** Internal string encodings support for libpqxx
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#if !defined(PQXX_H_ENCODINGS)
#  define PQXX_H_ENCODINGS

#  include <cassert>
#  include <iomanip>
#  include <string>
#  include <string_view>

#  include "pqxx/encoding_group.hxx"
#  include "pqxx/strconv.hxx"


namespace pqxx
{
PQXX_DECLARE_ENUM_CONVERSION(encoding_group);
} // namespace pqxx


namespace pqxx::internal
{
/// Return PostgreSQL's name for encoding enum value.
PQXX_PURE char const *name_encoding(int encoding_id) noexcept;

/// Convert libpq encoding enum value to its libpqxx group.
PQXX_PURE PQXX_LIBEXPORT encoding_group
enc_group(int /* libpq encoding ID */, sl);


/// Extract byte from buffer, return as unsigned char.
/** Don't generate out-of-line copies; they complicate profiling. */
PQXX_PURE PQXX_INLINE_ONLY constexpr inline unsigned char
get_byte(std::string_view buffer, std::size_t offset) noexcept
{
  assert(offset < std::size(buffer));
  return static_cast<unsigned char>(buffer[offset]);
}


/// Throw an error reporting that input text is not properly encoded.
[[noreturn]] PQXX_COLD PQXX_INLINE_COV inline void throw_for_encoding_error(
  char const *encoding_name, std::string_view buffer, std::size_t start,
  std::size_t count, sl loc)
{
  // C++23: Use std::ranges::views::join_with()?
  std::stringstream s;
  s << "Invalid byte sequence for encoding " << encoding_name << " at byte "
    << start << ": " << std::hex << std::setw(2) << std::setfill('0');
  for (std::size_t i{0}; i < count; ++i)
  {
    s << "0x" << static_cast<unsigned int>(get_byte(buffer, start + i));
    if (i + 1 < count)
      s << " ";
  }
  throw pqxx::argument_error{s.str(), loc};
}


/// Does value lie between bottom and top, inclusive?
/** Don't generate out-of-line copies; they complicate profiling. */
PQXX_PURE PQXX_INLINE_ONLY constexpr inline bool
between_inc(unsigned char value, unsigned bottom, unsigned top) noexcept
{
  return value >= bottom and value <= top;
}


/// Wrapper struct template for "find next glyph" functions.
/** When we use this, all we really want is a function pointer.  But that
 * won't work, because the template specialisation we use will only work (under
 * current C++ rules) for a struct or class, not for a function.
 */
template<encoding_group> struct glyph_scanner final
{
  /// Find the next glyph in `buffer` _after_ position `start`.
  /** The starting point must lie inside the view.
   */
  static constexpr inline std::size_t
  call(std::string_view, std::size_t start, sl);
};


/// Find any of the ASCII characters in `NEEDLE` in `haystack`.
/** Scans through `haystack` until it finds a single-byte character that
 * matches any of the values in `NEEDLE`.
 *
 * Returns the offset of the character it finds, or the end of the `haystack`
 * otherwise.
 */
template<encoding_group ENC, char... NEEDLE>
PQXX_INLINE_COV inline constexpr std::size_t
find_ascii_char(std::string_view haystack, std::size_t here, sl loc)
{
  // We only know how to search for ASCII characters.  It's an optimisation
  // assumption in the code below.
  static_assert((... and ((NEEDLE & 0x80) == 0)));

  auto const sz{std::size(haystack)};
  auto const data{std::data(haystack)};
  while (here < sz)
  {
    // Look up the next character boundary.  This can be quite costly, so we
    // desperately want the call inlined.
    auto next{glyph_scanner<ENC>::call(haystack, here, loc)};
    PQXX_ASSUME(next > here);

    // (For some reason gcc had a problem with a right-fold here.  But clang
    // was fine.)
    //
    // In all supported encodings, if a character's first byte is in the ASCII
    // range, that means it's a single-byte character.  It follows that when we
    // find a match at a position that's the beginning of a character, we do
    // not need to check that we're in a single-byte character.  We are.
    //
    // So, we only ever need to check each character's first byte, and if it
    // doesn't match, move on to the next character.
    //
    // As an optimisation for "ASCII-safe" encodings however, we just check
    // every byte in the text.  It's going to be faster than finding character
    // boundaries first.  In these encodings, a multichar byte never contains
    // any bytes in the ASCII range at all.
    if ((... or (data[here] == NEEDLE)))
      return here;

    // Nope, no hit.  Move on.
    here = next;
  }
  return sz;
}


template<> struct glyph_scanner<encoding_group::ascii_safe> final
{
  PQXX_INLINE_ONLY PQXX_PURE static constexpr std::size_t
  call(std::string_view, std::size_t start, sl) noexcept
  {
    return start + 1;
  }
};


// https://en.wikipedia.org/wiki/Big5#Organization
template<> struct glyph_scanner<encoding_group::big5> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    PQXX_ASSUME(start < std::size(buffer));

    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      return start + 1;

    auto const sz{std::size(buffer)};
    if (not between_inc(byte1, 0x81, 0xfe) or (start + 2 > sz)) [[unlikely]]
      throw_for_encoding_error("BIG5", buffer, start, 1, loc);

    auto const byte2{get_byte(buffer, start + 1)};
    if (
      not between_inc(byte2, 0x40, 0x7e) and
      not between_inc(byte2, 0xa1, 0xfe)) [[unlikely]]
      throw_for_encoding_error("BIG5", buffer, start, 2, loc);

    return start + 2;
  }
};


// https://en.wikipedia.org/wiki/GB_18030#Mapping
template<> struct glyph_scanner<encoding_group::gb18030> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    PQXX_ASSUME(start < std::size(buffer));
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      return start + 1;
    auto const sz{std::size(buffer)};
    if (byte1 == 0x80)
      throw_for_encoding_error("GB18030", buffer, start, sz - start, loc);

    if (start + 2 > sz) [[unlikely]]
      throw_for_encoding_error("GB18030", buffer, start, sz - start, loc);

    auto const byte2{get_byte(buffer, start + 1)};
    if (between_inc(byte2, 0x40, 0xfe))
    {
      if (byte2 == 0x7f) [[unlikely]]
        throw_for_encoding_error("GB18030", buffer, start, 2, loc);

      return start + 2;
    }

    if (start + 4 > sz) [[unlikely]]
      throw_for_encoding_error("GB18030", buffer, start, sz - start, loc);

    if (
      between_inc(byte2, 0x30, 0x39) and
      between_inc(get_byte(buffer, start + 2), 0x81, 0xfe) and
      between_inc(get_byte(buffer, start + 3), 0x30, 0x39))
      return start + 4;

    [[unlikely]] throw_for_encoding_error("GB18030", buffer, start, 4, loc);
  }
};


// https://en.wikipedia.org/wiki/GBK_(character_encoding)#Encoding
template<> struct glyph_scanner<encoding_group::gbk> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    PQXX_ASSUME(start < std::size(buffer));
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      return start + 1;

    auto const sz{std::size(buffer)};
    if (start + 2 > sz) [[unlikely]]
      throw_for_encoding_error("GBK", buffer, start, 1, loc);

    auto const byte2{get_byte(buffer, start + 1)};
    if (
      (between_inc(byte1, 0xa1, 0xa9) and between_inc(byte2, 0xa1, 0xfe)) or
      (between_inc(byte1, 0xb0, 0xf7) and between_inc(byte2, 0xa1, 0xfe)) or
      (between_inc(byte1, 0x81, 0xa0) and between_inc(byte2, 0x40, 0xfe) and
       byte2 != 0x7f) or
      (between_inc(byte1, 0xaa, 0xfe) and between_inc(byte2, 0x40, 0xa0) and
       byte2 != 0x7f) or
      (between_inc(byte1, 0xa8, 0xa9) and between_inc(byte2, 0x40, 0xa0) and
       byte2 != 0x7f) or
      (between_inc(byte1, 0xaa, 0xaf) and between_inc(byte2, 0xa1, 0xfe)) or
      (between_inc(byte1, 0xf8, 0xfe) and between_inc(byte2, 0xa1, 0xfe)) or
      (between_inc(byte1, 0xa1, 0xa7) and between_inc(byte2, 0x40, 0xa0) and
       byte2 != 0x7f))
      return start + 2;

    [[unlikely]] throw_for_encoding_error("GBK", buffer, start, 2, loc);
  }
};


/*
The PostgreSQL documentation claims that the JOHAB encoding is 1-3 bytes, but
"CJKV Information Processing" describes it (actually just the Hangul portion)
as "three five-bit segments" that reside inside 16 bits (2 bytes).

CJKV Information Processing by Ken Lunde, pg. 269:

  https://bit.ly/2BEOu5V
*/
template<> struct glyph_scanner<encoding_group::johab> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    PQXX_ASSUME(start < std::size(buffer));
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      return start + 1;

    auto const sz{std::size(buffer)};
    if (start + 2 > sz) [[unlikely]]
      throw_for_encoding_error("JOHAB", buffer, start, 1, loc);

    auto const byte2{get_byte(buffer, start)};
    if (
      (between_inc(byte1, 0x84, 0xd3) and
       (between_inc(byte2, 0x41, 0x7e) or between_inc(byte2, 0x81, 0xfe))) or
      ((between_inc(byte1, 0xd8, 0xde) or between_inc(byte1, 0xe0, 0xf9)) and
       (between_inc(byte2, 0x31, 0x7e) or between_inc(byte2, 0x91, 0xfe))))
      return start + 2;

    [[unlikely]] throw_for_encoding_error("JOHAB", buffer, start, 2, loc);
  }
};


// As far as I can tell, for the purposes of iterating the only difference
// between SJIS and SJIS-2004 is increased range in the first byte of two-byte
// sequences (0xEF increased to 0xFC).  Officially, that is; apparently the
// version of SJIS used by Postgres has the same range as SJIS-2004.  They both
// have increased range over the documented versions, not having the even/odd
// restriction for the first byte in 2-byte sequences.
//
// https://en.wikipedia.org/wiki/Shift_JIS#Shift_JIS_byte_map
// http://x0213.org/codetable/index.en.html
template<> struct glyph_scanner<encoding_group::sjis> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    PQXX_ASSUME(start < std::size(buffer));
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80 or between_inc(byte1, 0xa1, 0xdf))
      return start + 1;

    if (
      not between_inc(byte1, 0x81, 0x9f) and
      not between_inc(byte1, 0xe0, 0xfc)) [[unlikely]]
      throw_for_encoding_error("SJIS", buffer, start, 1, loc);

    auto const sz{std::size(buffer)};
    if (start + 2 > sz) [[unlikely]]
      throw_for_encoding_error("SJIS", buffer, start, sz - start, loc);

    auto const byte2{get_byte(buffer, start + 1)};
    if (byte2 == 0x7f) [[unlikely]]
      throw_for_encoding_error("SJIS", buffer, start, 2, loc);

    if (between_inc(byte2, 0x40, 0x9e) or between_inc(byte2, 0x9f, 0xfc))
      return start + 2;

    [[unlikely]] throw_for_encoding_error("SJIS", buffer, start, 2, loc);
  }
};


// https://en.wikipedia.org/wiki/Unified_Hangul_Code
template<> struct glyph_scanner<encoding_group::uhc> final
{
  PQXX_INLINE_ONLY static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    PQXX_ASSUME(start < std::size(buffer));
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      return start + 1;

    auto const sz{std::size(buffer)};
    if (start + 2 > sz) [[unlikely]]
      throw_for_encoding_error("UHC", buffer, start, sz - start, loc);

    auto const byte2{get_byte(buffer, start + 1)};
    if (between_inc(byte1, 0x80, 0xc6))
    {
      if (
        between_inc(byte2, 0x41, 0x5a) or between_inc(byte2, 0x61, 0x7a) or
        between_inc(byte2, 0x80, 0xfe))
        return start + 2;

      [[unlikely]] throw_for_encoding_error("UHC", buffer, start, 2, loc);
    }

    if (between_inc(byte1, 0xa1, 0xfe))
    {
      if (not between_inc(byte2, 0xa1, 0xfe)) [[unlikely]]
        throw_for_encoding_error("UHC", buffer, start, 2, loc);

      return start + 2;
    }

    throw_for_encoding_error("UHC", buffer, start, 1, loc);
  }
};


/// Look up a character search function for an encoding group.
/** We only define a few individual instantiations of this function, as needed.
 *
 * Returns a pointer to a function which looks for the first instance of any of
 * the ASCII characters in `NEEDLE`.  Returns its offset, or the end of the
 * `haystack` if it found none.
 */
template<char... NEEDLE>
PQXX_PURE
  PQXX_RETURNS_NONNULL PQXX_INLINE_COV constexpr inline char_finder_func *
  get_char_finder(encoding_group enc, sl loc)
{
  switch (enc)
  {
  case encoding_group::ascii_safe:
    return pqxx::internal::find_ascii_char<
      encoding_group::ascii_safe, NEEDLE...>;
  case encoding_group::big5:
    return pqxx::internal::find_ascii_char<encoding_group::big5, NEEDLE...>;
  case encoding_group::gb18030:
    return pqxx::internal::find_ascii_char<encoding_group::gb18030, NEEDLE...>;
  case encoding_group::gbk:
    return pqxx::internal::find_ascii_char<encoding_group::gbk, NEEDLE...>;
  case encoding_group::johab:
    return pqxx::internal::find_ascii_char<encoding_group::johab, NEEDLE...>;
  case encoding_group::sjis:
    return pqxx::internal::find_ascii_char<encoding_group::sjis, NEEDLE...>;
  case encoding_group::uhc:
    return pqxx::internal::find_ascii_char<encoding_group::uhc, NEEDLE...>;

  default:
    throw pqxx::internal_error{
      std::format(
        "Unexpected encoding group: {} (mapped from '{}').", to_string(enc),
        to_string(enc)),
      loc};
  }
}
} // namespace pqxx::internal
#endif
