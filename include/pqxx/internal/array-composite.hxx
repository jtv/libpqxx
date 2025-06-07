#if !defined(PQXX_ARRAY_COMPOSITE_HXX)
#  define PQXX_ARRAY_COMPOSITE_HXX

#  include <cassert>

#  include "pqxx/internal/encodings.hxx"
#  include "pqxx/strconv.hxx"

namespace pqxx::internal
{
// Find the end of a double-quoted string.
/** `input[pos]` must be the opening double quote.
 *
 * The backend double-quotes strings in composites or arrays, when needed.
 * Special characters are escaped using backslashes.
 *
 * Returns the offset of the first position after the closing quote.
 */
template<encoding_group ENC>
inline constexpr std::size_t
scan_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  PQXX_ASSUME((pos + 1) < std::size(input));
  assert(input[pos] == '"');

  auto const sz{std::size(input)};

  // The width in bytes of a single ASCII character.  In other words, one.
  constexpr std::size_t one_ascii_char{1u};

  // Skip over the opening double-quote.
  pos += one_ascii_char;
  while (pos < sz)
  {
    // No need to check for a multibyte character here: if it's multibyte, its
    // first byte won't match either of these ASCII characters.
    switch (input[pos])
    {
    case '\\':
      // Backslash escape.  Move on to the next character, so that at the end
      // of the iteration we'll skip right over it.
      pos += one_ascii_char;
      if (pos >= sz)
        throw argument_error{"Unexpected end of string: backslash.", loc};
      if ((input[pos] == '\\') or (input[pos] == '"'))
      {
        // As you'd expect: the backslash escapes a double-quote, or another
        // backslash.  Move past it, or the find_ascii_char<>() at the end of
        // the iteration will just stop here again.
        pos += one_ascii_char;
        if (pos >= sz)
          throw argument_error{
            "Unexected end of string: escape sequence.", loc};
      }
      break;

    case '"':
      pos += one_ascii_char;
      if (pos >= sz)
      {
        // Clear-cut case.  This is the closing quote and it's right at the end
        // of the input.
        return pos;
      }
      else if (input[pos] == '"')
      {
        // This is a doubled-up double-quote.  That's the other way of
        // escaping them.  Why can't this ever be simple?
        pos += one_ascii_char;
        if (pos >= sz)
          break;
      }
      else
      {
        // This was the closing quote (though not at the end of the input).
        return pos;
      }
    }

    // We've reached the end of one iteration without reaching the end of the
    // string.
    pos = find_ascii_char<ENC, '"', '\\'>(input, pos, loc);
  }

  // If we got here, we never found the closing double-quote.
  throw argument_error{
    "Missing closing double-quote: " + std::string{input}, loc};
}


// TODO: Needs version with caller-supplied buffer.
/// Un-quote and un-escape a double-quoted SQL string.
/** @param input Text.  The double-quoted string must start at offset `pos`,
 * and must end at the end of `input`.  So, truncate `input` before calling if
 * necessary.
 */
template<encoding_group ENC>
inline constexpr std::string
parse_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  std::string output;
  auto const end{std::size(input)};
  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  Or in the extreme opposite case, the real number could be
  // half that.  Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - pos - 2));

  // XXX: Use find_char<'"', '\\'>().
  using scanner = glyph_scanner<ENC>;
  auto here{scanner::call(input, pos, loc)},
    next{scanner::call(input, here, loc)};
  PQXX_ASSUME(here > pos);
  PQXX_ASSUME(next > here);
  while (here < end - 1)
  {
    // A backslash here is always an escape.  So is a double-quote, since we're
    // inside the double-quoted string.  In either case, we can just ignore the
    // escape character and use the next character.  This is the one redeeming
    // feature of SQL's escaping system.
    if ((next - here == 1) and (input[here] == '\\' or input[here] == '"'))
    {
      // Skip escape.
      here = next;
      next = scanner::call(input, here, loc);
      PQXX_ASSUME(next > here);
    }
    output.append(input.substr(here, next - here));
    here = next;
    next = scanner::call(input, here, loc);
    PQXX_ASSUME(next > here);
  }
  return output;
}


// XXX: Does this actually support escaping?  Does it need to?
/// Find the end of an unquoted string in an array or composite-type value.
/** Stops when it gets to the end of the input; or when it sees any of the
 * characters in STOP which has not been escaped.
 *
 * For array values, STOP is an array element separator (typically comma, or
 * semicolon), or a closing brace.  For a value of a composite type, STOP is a
 * comma or a closing parenthesis.
 */
template<encoding_group ENC, char... STOP>
inline constexpr std::size_t
scan_unquoted_string(std::string_view input, std::size_t pos, sl loc)
{
  using scanner = glyph_scanner<ENC>;
  // XXX: Use find_char<STOP...>().
  auto const sz{std::size(input)};
  auto next{scanner::call(input, pos, loc)};
  PQXX_ASSUME(pos < next);
  while ((pos < sz) and ((next - pos) > 1 or ((input[pos] != STOP) and ...)))
  {
    pos = next;
    next = scanner::call(input, pos, loc);
    PQXX_ASSUME(next > pos);
  }
  return pos;
}


/// Parse an unquoted array entry or cfield of a composite-type field.
/** @param input A view on the text, truncated at the end of the string.  So,
 *     the end of `input` must coincide with the end of the string.  Truncate
 *     before calling if necessary.
 * @param pos The string's starting offset within `input`.
 */
template<encoding_group ENC>
inline constexpr std::string_view
parse_unquoted_string(std::string_view input, std::size_t pos, sl)
{
  return input.substr(pos);
}


/// Parse a field of a composite-type value.
/** `T` is the C++ type of the field we're parsing, and `index` is its
 * zero-based number.
 *
 * Strip off the leading parenthesis or bracket yourself before parsing.
 * However, this function will parse the lcosing parenthesis or bracket.
 *
 * After a successful parse, `pos` will point at `std::end(text)`.
 *
 * For the purposes of parsing, ranges and arrays count as compositve values,
 * so this function supports parsing those.  If you specifically need a closing
 * parenthesis, check afterwards that `text` did not end in a bracket instead.
 *
 * @param index Index of the current field, zero-based.  It will increment for
 *     the next field.
 * @param input Full input text for the entire composite-type value.
 * @param pos Starting position (in `input`) of the field that we're parsing.
 *     After parsing, this will point at the beginning of the next field if
 *     there is one, or one position past the last character otherwise.
 * @param field Destination for the parsed value.
 * @param scan Glyph scanning function for the relevant encoding type.
 * @param last_field Number of the last field in the value (zero-based).  When
 *     parsing the last field, this will equal `index`.
 */
template<encoding_group ENC, typename T>
inline void parse_composite_field(
  std::size_t &index, std::string_view input, std::size_t &pos, T &field,
  std::size_t last_field, sl loc)
{
  assert(index <= last_field);
  // XXX: Use find_char().
  // XXX: Test for empty case?
  auto next{glyph_scanner<ENC>::call(input, pos, loc)};
  PQXX_ASSUME(next > pos);
  if ((next - pos) != 1)
    throw conversion_error{
      "Non-ASCII character in composite-type syntax.", loc};

  // Expect a field.
  switch (input[pos])
  {
  case ',':
  case ')':
  case ']':
    // The field is empty, i.e, null.
    if constexpr (nullness<T>::has_null)
      field = nullness<T>::null();
    else
      throw conversion_error{
        std::format(
          "Can't read composite field {}: C++ type {} does not support nulls.",
          to_string(index), name_type<T>()),
        loc};
    break;

  case '"': {
    auto const stop{scan_double_quoted_string<ENC>(input, pos, loc)};
    PQXX_ASSUME(stop > pos);
    auto const text{
      parse_double_quoted_string<ENC>(input.substr(0, stop), pos, loc)};
    field = from_string<T>(text);
    pos = stop;
  }
  break;

  default: {
    auto const stop{scan_unquoted_string<ENC, ',', ')', ']'>(input, pos, loc)};
    PQXX_ASSUME(stop >= pos);
    field = from_string<T>(input.substr(pos, stop - pos));
    pos = stop;
  }
  break;
  }

  // Expect a comma or a closing parenthesis.
  // XXX: Use find_char().
  next = glyph_scanner<ENC>::call(input, pos, loc);
  PQXX_ASSUME(next > pos);

  if ((next - pos) != 1)
    throw conversion_error{
      "Unexpected non-ASCII character after composite field: " +
        std::string{input},
      loc};

  if (index < last_field)
  {
    if (input[pos] != ',')
      throw conversion_error{
        "Found '" + std::string{input[pos]} +
          "' in composite value where comma was expected: " + std::data(input),
        loc};
  }
  else
  {
    if (input[pos] == ',')
      throw conversion_error{
        "Composite value contained more fields than the expected " +
          to_string(last_field) + ": " + std::data(input),
        loc};
    if (input[pos] != ')' and input[pos] != ']')
      throw conversion_error{
        "Composite value has unexpected characters where closing parenthesis "
        "was expected: " +
          std::string{input},
        loc};
    if (next != std::size(input))
      throw conversion_error{
        "Composite value has unexpected text after closing parenthesis: " +
          std::string{input},
        loc};
  }

  pos = next;
  ++index;
}


/// Pointer to an encoding-specific specialisation of parse_composite_field.
template<typename T>
using composite_field_parser = void (*)(
  std::size_t &index, std::string_view input, std::size_t &pos, T &field,
  std::size_t last_field, sl loc);


/// Look up implementation of parse_composite_field for ENC.
template<typename T>
composite_field_parser<T>
specialize_parse_composite_field(encoding_group enc, sl loc)
{
  switch (enc)
  {
  case encoding_group::UNKNOWN:
    throw usage_error{
      "Tried to parse array/composite without knowing its text encoding.",
      loc};

  case encoding_group::MONOBYTE:
    return parse_composite_field<encoding_group::MONOBYTE>;
  case encoding_group::BIG5:
    return parse_composite_field<encoding_group::BIG5>;
  case encoding_group::EUC_CN:
    return parse_composite_field<encoding_group::EUC_CN>;
  case encoding_group::EUC_JP:
    return parse_composite_field<encoding_group::EUC_JP>;
  case encoding_group::EUC_KR:
    return parse_composite_field<encoding_group::EUC_KR>;
  case encoding_group::EUC_TW:
    return parse_composite_field<encoding_group::EUC_TW>;
  case encoding_group::GB18030:
    return parse_composite_field<encoding_group::GB18030>;
  case encoding_group::GBK: return parse_composite_field<encoding_group::GBK>;
  case encoding_group::JOHAB:
    return parse_composite_field<encoding_group::JOHAB>;
  case encoding_group::MULE_INTERNAL:
    return parse_composite_field<encoding_group::MULE_INTERNAL>;
  case encoding_group::SJIS:
    return parse_composite_field<encoding_group::SJIS>;
  case encoding_group::UHC: return parse_composite_field<encoding_group::UHC>;
  case encoding_group::UTF8:
    return parse_composite_field<encoding_group::UTF8>;
  }
  throw internal_error{
    std::format("Unexpected encoding group code: {}.", to_string(enc)), loc};
}


/// Conservatively estimate buffer size needed for a composite field.
template<typename T>
inline std::size_t size_composite_field_buffer(T const &field)
{
  if constexpr (is_unquoted_safe<T>)
  {
    // Safe to copy, without quotes or escaping.  Drop the terminating zero.
    return size_buffer(field) - 1;
  }
  else
  {
    // + Opening quote.
    // + Field budget.
    // - Terminating zero.
    // + Escaping for each byte in the field's string representation.
    // - Escaping for terminating zero.
    // + Closing quote.
    return 1 + 2 * (size_buffer(field) - 1) + 1;
  }
}


template<typename T>
inline void write_composite_field(
  std::span<char> buf, std::size_t &pos, T const &field, sl loc)
{
  conversion_context const c{{}, loc};
  if constexpr (is_unquoted_safe<T>)
  {
    // No need for quoting or escaping.  Convert it straight into its final
    // place in the buffer, and "backspace" the trailing zero.
    pos += into_buf(buf.subspan(pos), field, c) - 1;
  }
  else
  {
    // The field may need escaping, which means we need an intermediate buffer.
    // To avoid allocating that at run time, we use the end of the buffer that
    // we have.
    auto const budget{size_buffer(field)};
    assert(budget < std::size(buf));
    // C++26: Use buf.at().
    buf[pos++] = '"';

    // Now escape buf into its final position.
    for (char const x : to_buf(buf.subspan(std::size(buf) - budget), field, c))
    {
      if ((x == '"') or (x == '\\'))
        // C++26: Use buf.at().
        buf[pos++] = '\\';

      // C++26: Use buf.at().
      buf[pos++] = x;
    }

    // C++26: Use buf.at().
    buf[pos++] = '"';
  }

  // C++26: Use buf.at().
  buf[pos++] = ',';
}
} // namespace pqxx::internal
#endif
