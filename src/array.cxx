/** Handling of SQL arrays.
 *
 * Copyright (c) 2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include <cassert>
#include <cstddef>
#include <cstring>
#include <utility>

#include "pqxx/array"
#include "pqxx/except"


namespace pqxx
{
/// Find the end of a single-quoted SQL string in an SQL array.
/** Returns the offset of the first character after the closing quote.
 */
std::string::size_type array_parser::scan_single_quoted_string() const
{
  // TODO: Use m_scan.
  auto here = m_pos;
  assert((here < m_end) and (m_input[here] == '\''));
  for (here++; m_input[here]; here++)
  {
    switch (m_input[here])
    {
    case '\'':
      // Escaped quote, or closing quote.
      here++;
      // If the next character is a quote, we've got a double single quote.
      // That's how SQL escapes embedded quotes in a string.  Terrible idea,
      // but it's what we have.
      if (m_input[here] != '\'') return here;
      // Otherwise, we've found the end of the string.  Return the address of
      // the very next byte.
      break;
    case '\\':
      // Backslash escape.  Skip ahead by one more character.
      here++;
      if (m_input[here] == '\0')
        throw argument_error{
          "SQL string ends in escape: " + std::string{m_input}};
      break;
    }
  }
  throw argument_error{"Null byte in SQL string: " + std::string{m_input}};
}


/// Parse a single-quoted SQL string: un-quote it and un-escape it.
std::string array_parser::parse_single_quoted_string(
	std::string::size_type end) const
{
  // TODO: Use m_scan.
  // There have to be at least 2 characters: the opening and closing quotes.
  assert(m_pos + 1 < end);
  assert(m_input[m_pos] == '\'');
  assert(m_input[end - 1] == '\'');

  std::string output;
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  In the worst case, the real number could be half that.
  // Usually it'll be a pretty close estimate.
  output.reserve(end - m_pos - 2);
  for (auto here = m_pos + 1; here < end - 1; here++)
  {
    // Skip escapes.
    if (m_input[here] == '\'' or m_input[here] == '\\') here++;
    output.push_back(m_input[here]);
  }

  return output;
}


/// Find the end of a double-quoted SQL string in an SQL array.
std::string::size_type array_parser::scan_double_quoted_string() const
{
  auto here = m_pos;
  assert(here < m_end);
  assert(m_input[here] == '"');
  for (here++; m_input[here]; here++)
  {
    switch (m_input[here])
    {
    case '\\':
      // Backslash escape.  Skip ahead by one more character.
      here++;
      if (m_input[here] == '\0')
        throw argument_error{
          "SQL string ends in escape: " + std::string{m_input}};
      break;
    case '"':
      return here + 1;
    }
  }
  throw argument_error{"Null byte in SQL string: " + std::string{m_input}};
}


/// Parse a double-quoted SQL string: un-quote it and un-escape it.
std::string array_parser::parse_double_quoted_string(
	std::string::size_type end) const
{
  // There have to be at least 2 characters: the opening and closing quotes.
  assert(m_pos + 1 < end);
  assert(m_input[m_pos] == '"');
  assert(m_input[end - 1] == '"');

  std::string output;
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  In the worst case, the real number could be half that.
  // Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - m_pos - 2));
  for (auto here = m_pos + 1; here < end - 1; here++)
  {
    // Skip escapes.
    if (m_input[here] == '\\') here++;
    output.push_back(m_input[here]);
  }

  return output;
}


/// Find the end of an unquoted string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string::size_type array_parser::scan_unquoted_string() const
{
  auto here = m_pos;
  assert(here < m_end);
  assert(m_input[here] != '\'');
  assert(m_input[here] != '"');

  while (
	m_input[here] != ',' and
	m_input[here] != ';' and
	m_input[here] != '}'
  )
    ++here;
  return here;
}


/// Parse an unquoted SQL string.
/** Here, the special unquoted value NULL means a null value, not a string
 * that happens to spell "NULL".
 */
std::string array_parser::parse_unquoted_string(
	std::string::size_type end) const
{
  return std::string{m_input + m_pos, m_input + end};
}


array_parser::array_parser(
	const char input[],
	internal::encoding_group enc) :
  m_input(input),
  m_end(input == nullptr ? 0 : std::strlen(input)),
  m_scan(internal::get_glyph_scanner(enc)),
  m_pos(0)
{
}


std::pair<array_parser::juncture, std::string>
array_parser::get_next()
{
  juncture found;
  std::string value;
  std::string::size_type end;

  if (m_input == nullptr)
  {
    found = juncture::done;
    end = 0;
  }
  else switch (m_input[m_pos])
  {
  case '\0':
    found = juncture::done;
    end = m_pos;
    break;
  case '{':
    found = juncture::row_start;
    end = m_pos + 1;
    break;
  case '}':
    found = juncture::row_end;
    end = m_pos + 1;
    break;
  case '\'':
    found = juncture::string_value;
    end = scan_single_quoted_string();
    value = parse_single_quoted_string(end);
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
      found = juncture::string_value;
    }
    break;
  }

  // Skip a field separator following a string (or null).
  if (end > 0 and (m_input[end] == ',' or m_input[end] == ';'))
    end++;

  m_pos = end;
  return std::make_pair(found, value);
}
} // namespace pqxx
