/** Handling of SQL arrays.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstddef>
#include <cstring>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/array.hxx"
#include "pqxx/except.hxx"
#include "pqxx/internal/array-composite.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/util.hxx"

#include "pqxx/internal/header-post.hxx"


namespace pqxx
{
/// Find the end of a double-quoted SQL string in an SQL array.
template<encoding_group ENC>
std::size_t array_parser::scan_double_quoted_string(sl loc) const
{
  return pqxx::internal::scan_double_quoted_string<ENC>(m_input, m_pos, loc);
}


/// Parse a double-quoted SQL string: un-quote it and un-escape it.
template<encoding_group ENC>
std::string
array_parser::parse_double_quoted_string(std::size_t end, sl loc) const
{
  // The only reason why we still let this substring start at the beginning
  // of the input is so that error messages can still provide a meaningful
  // offset.
  return pqxx::internal::parse_double_quoted_string<ENC>(
    m_input.substr(0, end), m_pos, loc);
}


/// Find the end of an unquoted string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
template<encoding_group ENC>
std::size_t array_parser::scan_unquoted_string(sl loc) const
{
  return pqxx::internal::scan_unquoted_string<ENC, ',', '}'>(
    m_input, m_pos, loc);
}


/// Parse an unquoted SQL string.
/** Here, the special unquoted value NULL means a null value, not a string
 * that happens to spell "NULL".
 */
template<encoding_group ENC>
std::string_view
array_parser::parse_unquoted_string(std::size_t end, sl loc) const
{
  return pqxx::internal::parse_unquoted_string<ENC>(
    m_input.substr(0, end), m_pos, loc);
}


array_parser::array_parser(std::string_view input, encoding_group enc) :
        m_input{input}, m_impl{specialize_for_encoding(enc, sl::current())}
{}


template<encoding_group ENC>
std::pair<array_parser::juncture, std::string>
array_parser::parse_array_step(sl loc)
{
  std::string value{};

  if (m_pos >= std::size(m_input))
    return std::make_pair(juncture::done, value);

  auto [found, end] = [this, &value, loc] {
    // We don't need to do any actual scanning yet, because we're looking for
    // specific ASCII characters and we're at the beginning of a character.
    // The first byte in a multibyte character will not be in the ASCII range.
    switch (m_input[m_pos])
    {
    case '\0': throw failure{"Unexpected zero byte in array.", loc};
    case '{': return std::tuple{juncture::row_start, m_pos + 1};
    case '}': return std::tuple{juncture::row_end, m_pos + 1};
    case '"': {
      // Double-quoted string.  This needs proper scanning.
      auto const endpoint = scan_double_quoted_string<ENC>(loc);
      value = parse_double_quoted_string<ENC>(endpoint, loc);
      return std::tuple{juncture::string_value, endpoint};
    }
    default: {
      // Unquoted string.  Needs proper scanning; in fact even the byte we're
      // currently inspecting may be the first in a multibyte character.
      auto const endpoint = scan_unquoted_string<ENC>(loc);
      value = parse_unquoted_string<ENC>(endpoint, loc);
      if (value == "NULL")
      {
        // In this one situation, as a special case, NULL means a null
        // field, not a string that happens to spell "NULL".
        value.clear();
        return std::tuple{juncture::null_value, endpoint};
      }
      else
      {
        // The normal case: we just parsed an unquoted string.  The value
        // is what we need.
        [[likely]] return std::tuple{juncture::string_value, endpoint};
      }
    }
    }
  }();

  // Skip a trailing field separator, if present.
  // No need to call the scanner here: if we're not at the end of the string,
  // we're at the beginning of a character.  And if the first byte of a
  // character is in the ASCII range, it's going to be an ASCII character.
  if (end < std::size(m_input) and (m_input[end] == ',')) [[unlikely]]
    ++end;

  m_pos = end;
  return std::make_pair(found, value);
}


array_parser::implementation
array_parser::specialize_for_encoding(encoding_group enc, sl loc)
{
#define PQXX_ENCODING_CASE(GROUP)                                             \
  case encoding_group::GROUP:                                                 \
    return &array_parser::parse_array_step<encoding_group::GROUP>

  switch (enc)
  {
  case encoding_group::unknown:
    throw usage_error{
      "Tried to parse array without knowing its encoding.", loc};

    PQXX_ENCODING_CASE(ascii_safe);
    PQXX_ENCODING_CASE(two_tier);
    PQXX_ENCODING_CASE(gb18030);
    PQXX_ENCODING_CASE(sjis);
  }
  [[unlikely]] throw pqxx::internal_error{
    std::format("Unsupported encoding code: {}.", to_string(enc)), loc};

#undef PQXX_ENCODING_CASE
}
} // namespace pqxx
