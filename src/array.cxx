/** Handling of SQL arrays.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
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
#include "pqxx/internal/concat.hxx"
#include "pqxx/strconv.hxx"
#include "pqxx/util.hxx"

#include "pqxx/internal/header-post.hxx"


namespace pqxx
{
/// Scan to next glyph in the buffer.  Assumes there is one.
template<pqxx::internal::encoding_group ENC>
[[nodiscard]] std::string::size_type
array_parser::scan_glyph(std::string::size_type pos) const
{
  return pqxx::internal::glyph_scanner<ENC>::call(
    std::data(m_input), std::size(m_input), pos);
}


/// Scan to next glyph in a substring.  Assumes there is one.
template<pqxx::internal::encoding_group ENC>
std::string::size_type array_parser::scan_glyph(
  std::string::size_type pos, std::string::size_type end) const
{
  return pqxx::internal::glyph_scanner<ENC>::call(
    std::data(m_input), end, pos);
}


/// Find the end of a double-quoted SQL string in an SQL array.
template<pqxx::internal::encoding_group ENC>
std::string::size_type array_parser::scan_double_quoted_string() const
{
  return pqxx::internal::scan_double_quoted_string<ENC>(
    std::data(m_input), std::size(m_input), m_pos);
}


/// Parse a double-quoted SQL string: un-quote it and un-escape it.
template<pqxx::internal::encoding_group ENC>
std::string
array_parser::parse_double_quoted_string(std::string::size_type end) const
{
  return pqxx::internal::parse_double_quoted_string<ENC>(
    std::data(m_input), end, m_pos);
}


/// Find the end of an unquoted string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
template<pqxx::internal::encoding_group ENC>
std::string::size_type array_parser::scan_unquoted_string() const
{
  return pqxx::internal::scan_unquoted_string<ENC, ',', '}'>(
    std::data(m_input), std::size(m_input), m_pos);
}


/// Parse an unquoted SQL string.
/** Here, the special unquoted value NULL means a null value, not a string
 * that happens to spell "NULL".
 */
template<pqxx::internal::encoding_group ENC>
std::string_view
array_parser::parse_unquoted_string(std::string::size_type end) const
{
  return pqxx::internal::parse_unquoted_string<ENC>(
    std::data(m_input), end, m_pos);
}


array_parser::array_parser(
  std::string_view input, internal::encoding_group enc) :
        m_input{input}, m_impl{specialize_for_encoding(enc)}
{}


template<pqxx::internal::encoding_group ENC>
std::pair<array_parser::juncture, std::string> array_parser::parse_array_step()
{
  std::string value{};

  if (m_pos >= std::size(m_input))
    return std::make_pair(juncture::done, value);

  auto [found, end] = [this, &value] {
    if (scan_glyph<ENC>(m_pos) - m_pos > 1)
    {
      // Non-ASCII unquoted string.
      auto const endpoint = scan_unquoted_string<ENC>();
      value = parse_unquoted_string<ENC>(endpoint);
      return std::tuple{juncture::string_value, endpoint};
    }
    else
      switch (m_input[m_pos])
      {
      case '\0': throw failure{"Unexpected zero byte in array."};
      case '{': return std::tuple{juncture::row_start, scan_glyph<ENC>(m_pos)};
      case '}': return std::tuple{juncture::row_end, scan_glyph<ENC>(m_pos)};
      case '"': {
        auto const endpoint = scan_double_quoted_string<ENC>();
        value = parse_double_quoted_string<ENC>(endpoint);
        return std::tuple{juncture::string_value, endpoint};
      }
      default: {
        auto const endpoint = scan_unquoted_string<ENC>();
        value = parse_unquoted_string<ENC>(endpoint);
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
          PQXX_LIKELY
          return std::tuple{juncture::string_value, endpoint};
        }
      }
      }
  }();

  // Skip a trailing field separator, if present.
  if (end < std::size(m_input))
  {
    auto next{scan_glyph<ENC>(end)};
    if (((next - end) == 1) and (m_input[end] == ','))
      PQXX_UNLIKELY
    end = next;
  }

  m_pos = end;
  return std::make_pair(found, value);
}


array_parser::implementation
array_parser::specialize_for_encoding(pqxx::internal::encoding_group enc)
{
  using encoding_group = pqxx::internal::encoding_group;

#define PQXX_ENCODING_CASE(GROUP)                                             \
  case encoding_group::GROUP:                                                 \
    return &array_parser::parse_array_step<encoding_group::GROUP>

  switch (enc)
  {
    PQXX_ENCODING_CASE(MONOBYTE);
    PQXX_ENCODING_CASE(BIG5);
    PQXX_ENCODING_CASE(EUC_CN);
    PQXX_ENCODING_CASE(EUC_JP);
    PQXX_ENCODING_CASE(EUC_KR);
    PQXX_ENCODING_CASE(EUC_TW);
    PQXX_ENCODING_CASE(GB18030);
    PQXX_ENCODING_CASE(GBK);
    PQXX_ENCODING_CASE(JOHAB);
    PQXX_ENCODING_CASE(MULE_INTERNAL);
    PQXX_ENCODING_CASE(SJIS);
    PQXX_ENCODING_CASE(UHC);
    PQXX_ENCODING_CASE(UTF8);
  }
  PQXX_UNLIKELY throw pqxx::internal_error{
    pqxx::internal::concat("Unsupported encoding code: ", enc, ".")};

#undef PQXX_ENCODING_CASE
}
} // namespace pqxx
