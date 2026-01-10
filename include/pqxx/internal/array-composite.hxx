#if !defined(PQXX_ARRAY_COMPOSITE_HXX)
#  define PQXX_ARRAY_COMPOSITE_HXX

#  include <cassert>

#  include "pqxx/util.hxx"

#  include "pqxx/internal/encodings.hxx"
#  include "pqxx/strconv.hxx"

namespace pqxx::internal
{
// The width in bytes of a single ASCII character.  In other words, one.
constexpr std::size_t one_ascii_char{1u};


// Find the end of a double-quoted string.
/** `input[pos]` must be the opening double quote.
 *
 * The backend double-quotes strings in composites or arrays, when needed.
 * Special characters are escaped using backslashes.
 *
 * Returns the offset of the first position after the closing quote.
 */
template<encoding_group ENC>
PQXX_INLINE_COV inline constexpr std::size_t
scan_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  assert(input[pos] == '"');
  auto const sz{std::size(input)};

  // Skip over the opening double-quote, and after that, any leading
  // "un-interesting" characters.
  pos = find_ascii_char<ENC, '"', '\\'>(input, pos + one_ascii_char, loc);
  while (pos < sz)
  {
    // No need to check for a multibyte character here: if it's multibyte, its
    // first byte won't match either of these ASCII characters.
    switch (input[pos])
    {
    case '"':
      // Is this the closing quote we're looking for?  Scan ahead to find out.
      pos += one_ascii_char;
      if (pos >= sz)
      {
        // Clear-cut case.  This is the closing quote and it's right at the end
        // of the input.
        return pos;
      }
      else if (input[pos] == '"')
      {
        // What we found is a doubled-up double-quote.  That's the other way of
        // escaping them.  Why can't this ever be simple?
        pos += one_ascii_char;
        if (pos >= sz)
          throw argument_error{
            "Unexpected end of string: double double-quote."};
      }
      else
      {
        // This was the closing quote (though not at the end of the input).
        // We are now at the one-past-end position.
        return pos;
      }
      break;

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
            "Unexpected end of string: escape sequence.", loc};
      }
      break;
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
PQXX_INLINE_COV inline constexpr std::string
parse_double_quoted_string(std::string_view input, std::size_t pos, sl loc)
{
  std::string output;
  auto const end{std::size(input)};
  assert((end - pos) > 1);
  assert(input[end - 1] == '"');

  // Maximum output size is same as the input size, minus the opening and
  // closing quotes.  Or in the extreme opposite case, the real number could be
  // half that.  Usually it'll be a pretty close estimate.
  output.reserve(std::size_t(end - pos - 2));

  auto const closing_quote{end - 1};

  // We're at the starting quote.  Skip it.
  assert(pos < closing_quote);
  assert(input[pos] == '"');
  pos += one_ascii_char;
  assert(pos <= closing_quote);

  // In theory, the closing quote should mean that there's no need for the
  // find_ascii_char() call to check for end-of-string inside its loop.  Not
  // sure whether the compiler will be smart enough to see that though.
  assert(input[closing_quote] == '"');

  while (pos < closing_quote)
  {
    auto const next{find_ascii_char<ENC, '"', '\\'>(input, pos, loc)};
    output.append(input.substr(pos, next - pos));
    pos = next;
    assert(pos <= closing_quote);
    assert((input[pos] == '"') or (input[pos] == '\\'));

    if (pos >= closing_quote)
      return output;

    // We're at either a backslash or a double-quote... and we're not at the
    // closing quote.  Therefore, we're at an escape character.  Skip it.
    pos += one_ascii_char;

    // We are now at the escaped character.
    // If the input has been scanned correctly, the string can't end here.
    assert(pos < closing_quote);

    if ((input[pos] == '"') or (input[pos] == '\\'))
    {
      // We know this is a single-byte character.  Append that (skipping the
      // escaping character) and move on to the next character.
      output.push_back(input[pos]);
      pos += one_ascii_char;
    }
    else
    {
      // This could be a multibyte character.  But no matter: we can let the
      // next iteration handle it like any run-of-the-mill character.
    }
  }
  assert(pos == closing_quote);

  return output;
}


/// Find the end of an unquoted string in an array or composite-type value.
/** Stops when it gets to the end of the input; or when it sees any of the
 * characters in STOP which has not been escaped.
 *
 * For array values, STOP is an array element separator (typically comma, or
 * semicolon), or a closing brace.  For a value of a composite type, STOP is a
 * comma or a closing parenthesis.
 */
template<encoding_group ENC, char... STOP>
PQXX_INLINE_COV inline constexpr std::size_t
scan_unquoted_string(std::string_view input, std::size_t pos, sl loc)
{
  return find_ascii_char<ENC, STOP...>(input, pos, loc);
}


/// Parse an unquoted array entry or cfield of a composite-type field.
/** @param input A view on the text, truncated at the end of the string.  So,
 *     the end of `input` must coincide with the end of the string.  Truncate
 *     before calling if necessary.
 * @param pos The string's starting offset within `input`.
 */
template<encoding_group ENC>
PQXX_INLINE_ONLY inline constexpr std::string_view
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
PQXX_INLINE_COV inline void parse_composite_field(
  std::size_t &index, std::string_view input, std::size_t &pos, T &field,
  std::size_t last_field, sl loc)
{
  assert(index <= last_field);
  assert(pos < std::size(input));
  conversion_context const c{ENC, loc};

  // Expect a field.
  switch (input[pos])
  {
  case ',':
  case ')':
  case ']':
    // The field is empty, i.e, null.
    if constexpr (has_null<T>())
      field = make_null<T>();
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
    field = from_string<T>(text, c);
    pos = stop;
  }
  break;

  default: {
    // Parse an unquoted string field.  It ends when we see a comma (meaning
    // there's a next field after it), or a closing parenthesis or bracket
    // (meaning we're at the last field).
    auto const stop{scan_unquoted_string<ENC, ',', ')', ']'>(input, pos, loc)};
    PQXX_ASSUME(stop >= pos);
    field = from_string<T>(input.substr(pos, stop - pos), c);
    pos = stop;
  }
  break;
  }

  // End of field.  Expect a comma or a closing parenthesis.

  if (index < last_field)
  {
    // There's another field coming after this one.
    if (input[pos] != ',')
      throw conversion_error{
        std::format(
          "Found '{}' in composite value where comma was expected: '{}.",
          input[pos], input),
        loc};
    pos += one_ascii_char;
  }
  else
  {
    // We're parsing the last field.
    if (input[pos] == ',')
      throw conversion_error{
        std::format(
          "Composite value contained more fields than the expected {}: '{}'.",
          to_string(last_field, c), std::data(input)),
        loc};
    if (input[pos] != ')' and input[pos] != ']')
      throw conversion_error{
        std::format(
          "Composite value has unexpected characters where closing "
          "parenthesis "
          "was expected: '{}'.",
          std::string{input}),
        loc};

    pos += one_ascii_char;

    if (pos != std::size(input))
      throw conversion_error{
        std::format(
          "Composite value has unexpected text after closing parenthesis: "
          "'{}'.",
          std::string{input}),
        loc};
  }
  ++index;
}


/// Pointer to an encoding-specific specialisation of parse_composite_field.
template<typename T>
using composite_field_parser = void (*)(
  std::size_t &index, std::string_view input, std::size_t &pos, T &field,
  std::size_t last_field, sl loc);


/// Look up implementation of parse_composite_field for ENC.
template<typename T>
PQXX_INLINE_COV inline constexpr composite_field_parser<T>
specialize_parse_composite_field(conversion_context const &c)
{
  switch (c.enc)
  {
  case encoding_group::unknown:
    throw usage_error{
      "Tried to parse array/composite without knowing its text encoding.",
      c.loc};

  case encoding_group::ascii_safe:
    return parse_composite_field<encoding_group::ascii_safe>;
  case encoding_group::two_tier:
    return parse_composite_field<encoding_group::two_tier>;
  case encoding_group::gb18030:
    return parse_composite_field<encoding_group::gb18030>;
  case encoding_group::sjis:
    return parse_composite_field<encoding_group::sjis>;
  }
  throw internal_error{
    std::format("Unexpected encoding group code: {}.", to_string(c.enc)),
    c.loc};
}


/// Conservatively estimate buffer size needed for a composite field.
template<typename T>
PQXX_INLINE_COV inline std::size_t size_composite_field_buffer(T const &field)
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
PQXX_INLINE_ONLY inline void write_composite_field(
  std::span<char> buf, std::size_t &pos, T const &field, ctx c)
{
  if constexpr (is_unquoted_safe<T>)
  {
    // No need for quoting or escaping.  Convert it straight into its final
    // place in the buffer.
    pos += into_buf(buf.subspan(pos), field, c);
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
    for (char const x : to_buf(buf.last(budget), field, c))
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


/// Write an SQL array representation into `buf`.
/** @return The number of bytes used, from the beginning of `buf`.  There is no
 * terminating zero.
 */
template<nonbinary_range TYPE>
[[nodiscard]] PQXX_INLINE_COV inline std::size_t array_into_buf(
  std::span<char> buf, TYPE const &value, std::size_t budget, ctx c)
{
  using elt_type = std::remove_cvref_t<value_type<TYPE>>;

  if (std::cmp_less(std::size(buf), budget))
    throw conversion_overrun{
      "Not enough buffer space to convert array to string.", c.loc};

  std::size_t here{0u};
  // C++26: Use buf.at().
  buf[here++] = '{';

  bool nonempty{false};
  for (auto const &elt : value)
  {
    static constexpr zview s_null{"NULL"};
    if (is_null(elt))
    {
      here = copy_chars<false>(s_null, buf, here, c.loc);
    }
    else if constexpr (is_sql_array<elt_type>)
    {
      // Render nested array in-place.
      here += pqxx::into_buf(buf.subspan(here), elt, c);
    }
    else if constexpr (is_unquoted_safe<elt_type>)
    {
      // No need to quote or escape.  Just convert the value straight into
      // its place in the array.
      here += pqxx::into_buf(buf.subspan(here), elt, c);
    }
    else
    {
      // Quote & escape.

      // C++26: Use buf.at().
      buf[here++] = '"';

      auto const sz{std::size(buf)}, elt_budget{pqxx::size_buffer(elt)};
      // Use the tail end of the destination buffer as an intermediate
      // buffer.
      assert(std::cmp_less(elt_budget, sz - here));
      auto const from{pqxx::to_buf(buf.last(elt_budget), elt, c)};
      auto const end{std::size(from)};
      auto const find{get_char_finder<'\\', '"'>(c.enc, c.loc)};

      // Copy the intermediate buffer into the final buffer, but escape
      // using backslashes.  The tricky part here is to handle encodings right.
      std::size_t i{0};
      while (i < end)
      {
        auto next{find(from, i, c.loc)};
        here =
          copy_chars<false>({std::data(from) + i, next - i}, buf, here, c.loc);
        if (next < end)
        {
          // We hit either a quote or a backslash.  Insert an escape
          // character (which is always a simple single ASCII byte).
          // C++26: Use buf.at().
          buf[here++] = '\\';
          // C++26: Use buf.at().
          // Copy the escaped character itself.  This is another simple single
          // ASCII byte.
          // TODO: Can we restructure this to leave that to the next iteration?
          buf[here++] = from[next++];
        }
        i = next;
      }
      // Copy any final text.
      here =
        copy_chars<false>({std::data(from) + i, end - i}, buf, here, c.loc);

      // C++26:Use buf.at().
      buf[here++] = '"';
    }
    // C++26:Use buf.at().
    buf[here++] = array_separator<elt_type>;
    nonempty = true;
  }

  // Erase that last comma, if present.
  if (nonempty)
    here--;

  // C++26:Use buf.at().
  buf[here++] = '}';

  return here;
}
} // namespace pqxx::internal
#endif
