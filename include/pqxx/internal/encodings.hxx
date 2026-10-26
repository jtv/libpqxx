/** Internal string encodings support for libpqxx
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_INTERNAL_ENCODINGS_HXX
#define PQXX_INTERNAL_ENCODINGS_HXX

#include <cassert>
#include <iomanip>
#include <string>
#include <string_view>
#include <utility>

#include "pqxx/encoding_group.hxx"
#include "pqxx/strconv.hxx"


namespace pqxx::internal
{
/// Return PostgreSQL's name for encoding enum value.
PQXX_PURE char const *name_encoding(int encoding_id) noexcept;

/// Convert libpq encoding enum value to its libpqxx group.
PQXX_PURE PQXX_LIBEXPORT encoding_group
enc_group(int /* libpq encoding ID */, sl);


/// Extract byte from buffer, return as unsigned char.
/** Don't generate out-of-line copies; they complicate profiling. */
PQXX_PURE PQXX_INLINE_ONLY PQXX_HOT constexpr inline unsigned char
get_byte(std::string_view buffer, std::size_t offset) noexcept
{
  // clang-tidy rule bug:
  // NOLINTNEXTLINE(cert-dcl03-c)
  assert(offset < std::size(buffer));
  return static_cast<unsigned char>(buffer[offset]);
}


/// Throw an error reporting that input text is not properly encoded.
/** @param encoding_name Either a name for the expected encoding, or a
 *    placeholder for it.
 * @param buffer The full input text.
 * @param start The starting offset for the incorrect character.
 * @param count The number of bytes in the incorrect character.  Must not
 *   exceed the size of the buffer.
 */
[[noreturn]] PQXX_COLD PQXX_LIBEXPORT PQXX_ZARGS void throw_for_encoding_error(
  char const *encoding_name, std::string_view buffer, std::size_t start,
  std::size_t count, sl loc);


/// Throw an error reporting that the input is truncated in mid-character.
/** This happens when a multibyte character is supposed to span more bytes
 * than there are left in the buffer.
 *
 * @param encoding_name Either a name for the expected encoding, or a
 *    placeholder for it.
 * @param buffer The full input text.
 * @param start The starting offset for the incorrect character.
 */
[[noreturn]] PQXX_COLD PQXX_LIBEXPORT PQXX_ZARGS void
throw_for_truncated_character(
  char const *encoding_name, std::string_view buffer, std::size_t start,
  sl loc);


/// Does value lie between bottom and top, inclusive?
/** Don't generate out-of-line copies; they complicate profiling. */
PQXX_PURE PQXX_INLINE_ONLY PQXX_HOT [[nodiscard]] constexpr inline bool
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
PQXX_INLINE_COV PQXX_HOT [[nodiscard]] inline constexpr std::size_t
find_ascii_char(std::string_view haystack, std::size_t here, sl loc)
{
  // We only know how to search for ASCII characters.  It's an optimisation
  // assumption in the code below.
  static_assert((... and ((NEEDLE & 0x80) == 0)));

  auto const sz{std::size(haystack)};
  char const *PQXX_RESTRICT const data{std::data(haystack)};
  while (here < sz)
  {
    // Look up the next character boundary.  This can be quite costly, so we
    // desperately want the call inlined.
    auto const next{glyph_scanner<ENC>::call(haystack, here, loc)};
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


/// ASCII, Latin-1, UTF-8, and the like.
/** These are the "ASCII-safe" encodings.  Safe in the sense that in correctly
 * encoded text, there can never be a byte that happens to have the same
 * numeric value has an ASCII character we might be looking for.
 *
 * This applies to UTF-8, Latin-* (iso-8859-*), other single-byte encodings,
 * and probably more.  So when we're scanning text in these encodings for one
 * or more special ASCII characters, we can just walk through it byte by byte.
 * That might _sound_ slower, but it's actually a lot more efficient than
 * constantly checking the values of the bytes and deciding when to jump ahead
 * by one or two bytes.
 */
template<> struct glyph_scanner<encoding_group::ascii_safe> final
{
  PQXX_INLINE_ONLY PQXX_PURE PQXX_HOT
    [[nodiscard]] static constexpr std::size_t
    call(std::string_view, std::size_t start, sl) noexcept
  {
    return start + 1;
  }
};


/// Common encoding pattern: 1-byte ASCII, or 2-byte starting with high byte.
/** Both BIG5 and Unified Hangul Code (UHC) work like this.  The details vary,
 * with both having different sub-ranges where no valid characters exist, but
 * we don't check for those.  We simply don't care.  Caring is inefficient.
 *
 * What we do care about is that when a byte has a value that looks like a
 * special ASCII character we're trying to find, we know exactly whether it is
 * that ASCII character, or just a byte inside a multibyte character.
 */
template<> struct glyph_scanner<encoding_group::two_tier> final
{
  PQXX_INLINE_ONLY [[nodiscard]] static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
    {
      // Single-byte ASCII subset.
      return start + 1;
    }
    else if (start + 2 <= std::size(buffer))
    {
      // Two-byte character.  Not all combinations are valid, but that's not
      // our concern.  All that matters to libpqxx is that it not mistake an
      // ASCII-like value in the second byte for a special character, or vice
      // versa.
      return start + 2;
    }
    else
    {
      // We do need to ensure that the string does not end in the middle of
      // a character, or an attacker could "steal" a special ASCII character
      // that comes directly after the end of the input, and escape the bounds
      // of the text that way.
      [[unlikely]] throw_for_truncated_character(
        "variable-width two-byte encoding", buffer, start, loc);
    }
  }
};


/// GB18030 and its older subsets, including GBK.
/** Chinese variable-width encoding of up to 4 bytes per character.
 *
 * See https://en.wikipedia.org/wiki/GB_18030#Mapping
 */
template<> struct glyph_scanner<encoding_group::gb18030> final
{
  PQXX_INLINE_ONLY [[nodiscard]] static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      return start + 1;
    auto const sz{std::size(buffer)};
    if (byte1 == 0x80)
      throw_for_encoding_error("GB18030", buffer, start, sz - start, loc);

    if (start + 2 > sz) [[unlikely]]
      throw_for_truncated_character("GB18030", buffer, start, loc);

    auto const byte2{get_byte(buffer, start + 1)};
    if (between_inc(byte2, 0x40, 0xfe))
    {
      if (byte2 == 0x7f) [[unlikely]]
        throw_for_encoding_error("GB18030", buffer, start, 2, loc);

      return start + 2;
    }

    if (start + 4 > sz) [[unlikely]]
      throw_for_truncated_character("GB18030", buffer, start, loc);

    if (
      between_inc(byte2, 0x30, 0x39) and
      between_inc(get_byte(buffer, start + 2), 0x81, 0xfe) and
      between_inc(get_byte(buffer, start + 3), 0x30, 0x39))
      return start + 4;

    [[unlikely]] throw_for_encoding_error("GB18030", buffer, start, 4, loc);
  }
};


/// Shift-JIS family of encodings.
/** These are variable-width encodings with 1-byte and 2-byte characters, but
 * with a twist: Katakana is mapped in the above-ASCII range _as single-byte
 * characters._
 *
 * If it weren't for that twist, this would be just like @ref two_tier.
 */
template<> struct glyph_scanner<encoding_group::sjis> final
{
  PQXX_INLINE_ONLY [[nodiscard]] static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    auto const byte1{get_byte(buffer, start)};
    if (byte1 < 0x80)
      // ASCII subset (though some characters changed).
      return start + 1;
    if (between_inc(byte1, 0xa1, 0xdf))
      // Katakana, also single-byte characters.
      return start + 1;

    // We're a bit strict at checking the first byte, because this is a
    // relatively complex encoding.  We don't want to get fooled by some
    // extension we don't know about.  An error and a user complaint is still
    // better than a lurking bug.
    if (
      not between_inc(byte1, 0x81, 0x9f) and
      not between_inc(byte1, 0xe0, 0xfc)) [[unlikely]]
      throw_for_encoding_error("SJIS", buffer, start, 1, loc);

    if (start + 2 > std::size(buffer)) [[unlikely]]
      throw_for_truncated_character("SJIS", buffer, start, loc);

    return start + 2;
  }
};


/// Shift_JIS-2004.
/** This old version of @ref sjis has one nasty twist; see
 * @ref sjis_2004_ascii_alias().  Structurally though we can treat it as
 * regular SJIS.
 */
template<> struct glyph_scanner<encoding_group::shift_jis_2004> final
{
  PQXX_INLINE_ONLY [[nodiscard]] static constexpr std::size_t
  call(std::string_view buffer, std::size_t start, sl loc)
  {
    return glyph_scanner<encoding_group::sjis>::call(buffer, start, loc);
  }
};


/// If this Shift_JIS-2004 character is a stand-in for an ASCII one, which?
/** The server converts text from the client encoding to the database encoding
 * _before_ parsing it.  In Shift_JIS-2004 the two-byte sequence 0x81 0x5f
 * comes out of that conversion as an ASCII backslash (as well as 0x81 0xb0 as
 * an ASCII tilde).  Escaping happens after that, during parsing, so we have to
 * treat this special spelling of the backslash just like a regular backslash.
 *
 * @param text Input text.
 * @param start The offset where the character starts.
 * @param next The offset where the next character starts, or text `end()`.
 * @return The ASCII character which the server will see in place of this
 *   character, or zero if it's any other character.
 */
PQXX_PURE PQXX_INLINE_ONLY [[nodiscard]] constexpr inline char
sjis_2004_ascii_alias(
  std::string_view text, std::size_t start, std::size_t next) noexcept
{
  if ((next - start == 2) and (get_byte(text, start) == 0x81))
    switch (get_byte(text, start + 1))
    {
    case 0x5f: return '\\';
    case 0xb0: return '~';
    default: break;
    }
  return '\0';
}


/// Find the next character that's one of `NEEDLE`.
/** In text that we're about to send to the server, when looking for
 * characters that need escaping, use this to find the first character in
 * `haystack` after offset `here` which occurs in `NEEDLE`.
 *
 * We can't just use `find_ascii_char()` for this scenario because of one nasty
 * case in Shift JIS-2004.
 *
 * @return Offset of the first such character it finds, or `haystack.end()`.
 * Use @ref special_char() to find out which ASCII character the server will
 * see there, and how many bytes in `haystack` the original character takes up.
 */
template<encoding_group ENC, char... NEEDLE>
PQXX_INLINE_COV [[nodiscard]] inline constexpr std::size_t
find_escapable_char(std::string_view haystack, std::size_t here, sl loc)
{
  static_assert((NEEDLE == '\\') or ...);

  if constexpr (ENC == encoding_group::shift_jis_2004)
  {
    // Nasty special case: there's a 2-byte alias for the backslash.
    auto const sz{std::size(haystack)};
    char const *PQXX_RESTRICT const data{std::data(haystack)};
    while (here < sz)
    {
      auto const next{glyph_scanner<ENC>::call(haystack, here, loc)};
      PQXX_ASSUME(next > here);
      if ((... or (data[here] == NEEDLE)))
        return here;
      auto const alias{sjis_2004_ascii_alias(haystack, here, next)};
      if ((... or (alias == NEEDLE)))
        return here;
      here = next;
    }
    return sz;
  }
  else
  {
    // In all other encodings, the server reads the bytes as we see them.
    return find_ascii_char<ENC, NEEDLE...>(haystack, here, loc);
  }
}


/// Which special character did @ref find_escapable_char() find?
/** Call this on the offset returned by a finder obtained through
 * @ref get_escape_finder().  Normally the special character is a single ASCII
 * byte, but in Shift_JIS-2004 it can also be a particular two-byte character
 * which the server will convert to ASCII.
 *
 * @return The ASCII character as the server will see it, and the number of
 * bytes the character occupies in `text`.
 */
PQXX_PURE PQXX_INLINE_ONLY PQXX_HOT
  [[nodiscard]] constexpr inline std::pair<char, std::size_t>
  special_char(std::string_view text, std::size_t here) noexcept
{
  // This function's call sites aren't statically aware of the encoding.  It
  // might be preferable to move the special SJIS-2004 case into an "if
  // constexpr" block, but we get by without it because this function only gets
  // called at an escapable character, and the only interpretation of a high
  // byte there is that we hit exactly that one SJIS-2004 case.
  // TODO: Can we move call sites so we can use "if constexpr" here?
  if (get_byte(text, here) < 0x80) [[likely]]
    return {text[here], 1u};
  else
    return {sjis_2004_ascii_alias(text, here, here + 2), 2u};
}


/// Look up a character search function for an encoding group.
/** We only define a few individual instantiations of this function, as needed.
 *
 * Returns a pointer to a function which looks for the first instance of any of
 * the ASCII characters in `NEEDLE`.  Returns its offset, or the end of the
 * `haystack` if it found none.
 *
 * @warn All of the characters in `NEEDLE` need to be ASCII characters.
 */
template<char... NEEDLE>
PQXX_PURE PQXX_RETURNS_NONNULL PQXX_INLINE_COV
  [[nodiscard]] constexpr inline char_finder_func *
  get_char_finder(encoding_group enc, sl loc)
{
  // All characters in NEEDLE must be ASCII.
  static_assert((... and (static_cast<unsigned char>(NEEDLE) < 0x80)));

  switch (enc)
  {
  case encoding_group::unknown:
    throw pqxx::argument_error{
      "Tried to read text without knowing its encoding.", loc};

  case encoding_group::ascii_safe:
    return pqxx::internal::find_ascii_char<
      encoding_group::ascii_safe, NEEDLE...>;
  case encoding_group::two_tier:
    return pqxx::internal::find_ascii_char<
      encoding_group::two_tier, NEEDLE...>;
  case encoding_group::gb18030:
    return pqxx::internal::find_ascii_char<encoding_group::gb18030, NEEDLE...>;
  case encoding_group::sjis:
    return pqxx::internal::find_ascii_char<encoding_group::sjis, NEEDLE...>;
  case encoding_group::shift_jis_2004:
    return pqxx::internal::find_ascii_char<
      encoding_group::shift_jis_2004, NEEDLE...>;

  default:
    throw pqxx::internal_error{
      std::format(
        "Unexpected encoding group: {}.",
        static_cast<std::underlying_type_t<encoding_group>>(enc)),
      loc};
  }
}


/// Look up a function for finding characters that need escaping.
/** This is like @ref get_char_finder(), but for text that we're going to send
 * to the server.  The function it returns also finds (in addition to ASCII
 * special characters) any multibyte characters which the server will convert
 * to one of the ASCII characters in `NEEDLE` before parsing the text.  See
 * @ref find_escapable_char().
 *
 * When the finder returns an offset inside the `haystack`, use
 * @ref special_char() to see which ASCII character the server will read there,
 * and how many bytes in the input to skip.
 */
template<char... NEEDLE>
PQXX_PURE PQXX_RETURNS_NONNULL PQXX_INLINE_COV
  [[nodiscard]] constexpr inline char_finder_func *
  get_escape_finder(encoding_group enc, sl loc)
{
  if (enc == encoding_group::shift_jis_2004)
    return pqxx::internal::find_escapable_char<
      encoding_group::shift_jis_2004, NEEDLE...>;
  else
    return get_char_finder<NEEDLE...>(enc, loc);
}
} // namespace pqxx::internal
#endif
