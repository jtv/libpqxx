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
 * Returns the address of the first character after the closing quote.
 */
const char*scan_single_quoted_string(const char begin[])
{
  const char *here = begin;
  assert(*here == '\'');
  for (here++; *here; here++)
  {
    switch (*here)
    {
    case '\'':
      // Escaped quote, or closing quote.
      here++;
      // If the next character is a quote, we've got a double single quote.
      // That's how SQL escapes embedded quotes in a string.  Terrible idea,
      // but it's what we have.
      if (*here != '\'') return here;
      // Otherwise, we've found the end of the string.  Return the address of
      // the very next byte.
      break;
    case '\\':
      // Backslash escape.  Skip ahead by one more character.
      here++;
      if (!*here)
        throw pqxx::argument_error(
          "SQL string ends in escape: " + std::string(begin));
      break;
    }
  }
  throw pqxx::argument_error(
      "Null byte in SQL string: " + std::string(begin));
}


/// Parse a single-quoted SQL string: un-quote it and un-escape it.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string parse_single_quoted_string(const char begin[], const char end[])
{
  // There have to be at least 2 characters: the opening and closing quotes.
  assert(begin + 1 < end);
  assert(*begin == '\'');
  assert(*(end - 1) == '\'');

  std::string output;
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  In the worst case, the real number could be half that.
  // Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - begin - 2));
  for (const char *here = begin + 1; here < end - 1; here++)
  {
    // Skip escapes.
    if (*here == '\'' || *here == '\\') here++;
    auto c = *here;
    output.push_back(c);
  }

  return output;
}


/// Find the end of a double-quoted SQL string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
const char *scan_double_quoted_string(const char begin[])
{
  const char *here = begin;
  assert(*here == '"');
  for (here++; *here; here++)
  {
    switch (*here)
    {
    case '\\':
      // Backslash escape.  Skip ahead by one more character.
      here++;
      if (!*here)
        throw pqxx::argument_error(
          "SQL string ends in escape: " + std::string(begin));
      break;
    case '"':
      return here + 1;
    }
  }
  throw pqxx::argument_error(
      "Null byte in SQL string: " + std::string(begin));
}


/// Parse a double-quoted SQL string: un-quote it and un-escape it.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
std::string parse_double_quoted_string(const char begin[], const char end[])
{
  // There have to be at least 2 characters: the opening and closing quotes.
  assert(begin + 1 < end);
  assert(*begin == '"');
  assert(*(end - 1) == '"');

  std::string output;
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  In the worst case, the real number could be half that.
  // Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - begin - 2));
  for (const char *here = begin + 1; here < end - 1; here++)
  {
    // Skip escapes.
    if (*here == '\\') here++;
    auto c = *here;
    output.push_back(c);
  }

  return output;
}


/// Find the end of an unquoted string in an SQL array.
/** Assumes UTF-8 or an ASCII-superset single-byte encoding.
 */
const char *scan_unquoted_string(const char begin[])
{
  assert(*begin != '\'');
  assert(*begin != '"');

  const char *p;
  for (
        p = begin;
        *p != ',' && *p != ';' && *p != '}';
        p++);
  return p;
}


/// Parse an unquoted SQL string.
/** Here, the special unquoted value NULL means a null value, not a string
 * that happens to spell "NULL".
 */
std::string parse_unquoted_string(const char begin[], const char end[])
{
  return std::string(begin, end);
}

} // namespace


namespace pqxx
{
array_parser::array_parser(const char input[]) : m_pos(input)
{
}


std::pair<array_parser::juncture, std::string>
array_parser::get_next()
{
  pqxx::array_parser::juncture found;
  std::string value;
  const char *end;

  if (m_pos == nullptr)
  {
    found = juncture::done;
    end = nullptr;
  }
  else switch (*m_pos)
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
    end = scan_single_quoted_string(m_pos);
    value = parse_single_quoted_string(m_pos, end);
    break;
  case '"':
    found = juncture::string_value;
    end = scan_double_quoted_string(m_pos);
    value = parse_double_quoted_string(m_pos, end);
    break;
  default:
    end = scan_unquoted_string(m_pos);
    value = parse_unquoted_string(m_pos, end);
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
  if (end != nullptr && (*end == ',' || *end == ';'))
    end++;

  m_pos = end;
  return std::make_pair(found, value);
}
} // namespace pqxx
