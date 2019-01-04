/** Handling of SQL arrays.
 *
 * Copyright (c) 2018, Jeroen T. Vermeulen.
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


using namespace pqxx::internal;


namespace
{

/// Find the end of a single-quoted SQL string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 * This is used for array parsing, where the database may send us strings
 * stored in the array, in a quoted and escaped format.
 *
 * Returns the offset of the first character after the closing quote.
 */
std::string::size_type scan_single_quoted_string(
	const char buffer[],
	std::string::size_type pos,
	std::string::size_type end)
{
  // TODO: Use glyph_scanner_func.
  assert((pos < end) and (buffer[pos] == '\''));
  for (pos++; buffer[pos]; pos++)
  {
    switch (buffer[pos])
    {
    case '\'':
      // Escaped quote, or closing quote.
      pos++;
      // If the next character is a quote, we've got a double single quote.
      // That's how SQL escapes embedded quotes in a string.  Terrible idea,
      // but it's what we have.
      if (buffer[pos] != '\'') return pos;
      // Otherwise, we've found the end of the string.  Return the address of
      // the very next byte.
      break;
    case '\\':
      // Backslash escape.  Skip ahead by one more character.
      pos++;
      if (buffer[pos] == '\0')
        throw pqxx::argument_error{
          "SQL string ends in escape: " + std::string{buffer}};
      break;
    }
  }
  throw pqxx::argument_error{
      "Null byte in SQL string: " + std::string{buffer}};
}


/// Parse a single-quoted SQL string: un-quote it and un-escape it.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string parse_single_quoted_string(
	const char buffer[],
        std::string::size_type pos,
	std::string::size_type end)
{
  // There have to be at least 2 characters: the opening and closing quotes.
// TODO: Check for correct buffer size.
  assert(pos + 1 < end);
  assert(buffer[pos] == '\'');
  assert(buffer[end - 1] == '\'');

  std::string output;
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  In the worst case, the real number could be half that.
  // Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - pos - 2));
  for (auto here = pos + 1; here < end - 1; here++)
  {
    // Skip escapes.
    if (buffer[here] == '\'' or buffer[here] == '\\') here++;
    output.push_back(buffer[here]);
  }

  return output;
}


/// Find the end of a double-quoted SQL string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string::size_type scan_double_quoted_string(
	const char buffer[],
	std::string::size_type pos,
	std::string::size_type /* end */)
{
  assert(buffer[pos] == '"');
  for (pos++; buffer[pos]; pos++)
  {
    switch (buffer[pos])
    {
    case '\\':
      // Backslash escape.  Skip ahead by one more character.
      pos++;
      if (buffer[pos] == '\0')
        throw pqxx::argument_error{
          "SQL string ends in escape: " + std::string{buffer}};
      break;
    case '"':
      return pos + 1;
    }
  }
  throw pqxx::argument_error{
      "Null byte in SQL string: " + std::string{buffer}};
}


/// Parse a double-quoted SQL string: un-quote it and un-escape it.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string parse_double_quoted_string(
	const char buffer[],
	std::string::size_type pos,
	std::string::size_type end)
{
  // There have to be at least 2 characters: the opening and closing quotes.
  assert(pos + 1 < end);
  assert(buffer[pos] == '"');
  assert(buffer[end - 1] == '"');

  std::string output;
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  In the worst case, the real number could be half that.
  // Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - pos - 2));
  for (auto here = pos + 1; here < end - 1; here++)
  {
    // Skip escapes.
    if (buffer[here] == '\\') here++;
    output.push_back(buffer[here]);
  }

  return output;
}


/// Find the end of an unquoted string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string::size_type scan_unquoted_string(
	const char buffer[],
	std::string::size_type pos,
	std::string::size_type end)
{
  assert(pos < end);
  assert(buffer[pos] != '\'');
  assert(buffer[pos] != '"');

  while (buffer[pos] != ',' and buffer[pos] != ';' and buffer[pos] != '}')
    ++pos;
  return pos;
}


/// Parse an unquoted SQL string.
/** Here, the special unquoted value NULL means a null value, not a string
 * that happens to spell "NULL".
 */
std::string parse_unquoted_string(
	const char buffer[],
	std::string::size_type pos,
	std::string::size_type end)
{
  return std::string{buffer + pos, buffer + end};
}

} // namespace


namespace pqxx
{
array_parser::array_parser(const char input[]) :
  m_input(input),
  m_end(input == nullptr ? 0 : std::strlen(input)),
  m_pos(0)
{
}


std::pair<array_parser::juncture, std::string>
array_parser::get_next()
{
  pqxx::array_parser::juncture found;
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
    end = scan_single_quoted_string(m_input, m_pos, m_end);
    value = parse_single_quoted_string(m_input, m_pos, end);
    break;
  case '"':
    found = juncture::string_value;
    end = scan_double_quoted_string(m_input, m_pos, m_end);
    value = parse_double_quoted_string(m_input, m_pos, end);
    break;
  default:
    end = scan_unquoted_string(m_input, m_pos, m_end);
    value = parse_unquoted_string(m_input, m_pos, end);
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
