/** Handling of SQL arrays.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
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
[[nodiscard]] std::string::size_type
array_parser::scan_glyph(std::string::size_type pos) const
{
  return m_scan(std::data(m_input), std::size(m_input), pos);
}


/// Scan to next glyph in a substring.  Assumes there is one.
std::string::size_type array_parser::scan_glyph(
  std::string::size_type pos, std::string::size_type end) const
{
  return m_scan(std::data(m_input), end, pos);
}


/// Find the end of a double-quoted SQL string in an SQL array.
std::string::size_type array_parser::scan_double_quoted_string() const
{
  return pqxx::internal::scan_double_quoted_string(
    std::data(m_input), std::size(m_input), m_pos, m_scan);
}


/// Parse a double-quoted SQL string: un-quote it and un-escape it.
std::string
array_parser::parse_double_quoted_string(std::string::size_type end) const
{
  return pqxx::internal::parse_double_quoted_string(
    std::data(m_input), end, m_pos, m_scan);
}


/// Find the end of an unquoted string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string::size_type array_parser::scan_unquoted_string() const
{
  return pqxx::internal::scan_unquoted_string<',', ';', '}'>(
    std::data(m_input), std::size(m_input), m_pos, m_scan);
}


/// Parse an unquoted SQL string.
/** Here, the special unquoted value NULL means a null value, not a string
 * that happens to spell "NULL".
 */
std::string
array_parser::parse_unquoted_string(std::string::size_type end) const
{
  return pqxx::internal::parse_unquoted_string(
    std::data(m_input), end, m_pos, m_scan);
}


array_parser::array_parser(
  std::string_view input, internal::encoding_group enc) :
        m_input(input), m_scan(internal::get_glyph_scanner(enc))
{}


std::pair<array_parser::juncture, std::string> array_parser::get_next()
{
  std::string value;

  if (m_pos >= std::size(m_input))
    return std::make_pair(juncture::done, value);

  juncture found;
  std::string::size_type end;

  if (scan_glyph(m_pos) - m_pos > 1)
  {
    // Non-ASCII unquoted string.
    end = scan_unquoted_string();
    value = parse_unquoted_string(end);
    found = juncture::string_value;
  }
  else
    switch (m_input[m_pos])
    {
    case '\0': throw failure{"Unexpected zero byte in array."};
    case '{':
      found = juncture::row_start;
      end = scan_glyph(m_pos);
      break;
    case '}':
      found = juncture::row_end;
      end = scan_glyph(m_pos);
      break;
    case '"':
      found = juncture::string_value;
      end = scan_double_quoted_string();
      value = parse_double_quoted_string(end);
      break;
    default:
      end = scan_unquoted_string();
      value = parse_unquoted_string(end);
      if (value == "NULL")
      {
        // In this one situation, as a special case, NULL means a null field,
        // not a string that happens to spell "NULL".
        value.clear();
        found = juncture::null_value;
      }
      else
      {
        // The normal case: we just parsed an unquoted string.  The value is
        // what we need.
        PQXX_LIKELY
        found = juncture::string_value;
      }
      break;
    }

  // Skip a trailing field separator, if present.
  if (end < std::size(m_input))
  {
    auto next{scan_glyph(end)};
    if (next - end == 1 and (m_input[end] == ',' or m_input[end] == ';'))
      PQXX_UNLIKELY
    end = next;
  }

  m_pos = end;
  return std::make_pair(found, value);
}
} // namespace pqxx
