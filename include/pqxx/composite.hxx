#ifndef PQXX_H_COMPOSITE
#define PQXX_H_COMPOSITE

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include "pqxx/internal/array-composite.hxx"
#include "pqxx/util.hxx"

namespace pqxx
{
/// Parse a string representation of a value of a composite type.
/** @warning This code is still experimental.  Use with care.
 *
 * You may use this as a helper while implementing your own @ref string_traits
 * for a composite type.
 *
 * This function interprets `text` as the string representation of a value of
 * some composite type, and sets each of `fields` to the respective values of
 * its fields.  The field types must be copy-assignable.
 *
 * The number of fields must match the number of fields in the composite type,
 * and there must not be any other text in the input.  The function is meant to
 * handle any value string that the backend can produce, but not necessarily
 * every valid alternative spelling.
 *
 * Fields in composite types can be null.  When this happens, the C++ type of
 * the corresponding field reference must be of a type that can handle nulls.
 * If you are working with a type that does not have an inherent null value,
 * such as e.g. `int`, consider using `std::optional`.
 */
template<typename... T>
inline void parse_composite(ctx c, std::string_view text, T &...fields)
{
  static constexpr auto num_fields{sizeof...(T)};
  static_assert(num_fields > 0);

  auto const data{std::data(text)};
  auto const size{std::size(text)};
  if (size == 0)
    throw conversion_error{
      "Cannot parse composite value from empty string.", c.loc};

  if (data[0] != '(')
    throw conversion_error{
      std::format("Invalid composite value string: '{}'.", text), c.loc};

  std::size_t here{1};

  std::size_t index{0};
  (pqxx::internal::specialize_parse_composite_field<T>(c)(
     index, text, here, fields, num_fields - 1, c.loc),
   ...);
  if (here != std::size(text))
    throw conversion_error{
      std::format(
        "Composite value did not end at the closing parenthesis: '{}'.", text),
      c.loc};
  if (text[here - 1] != ')')
    throw conversion_error{
      std::format("Composite value did not end in parenthesis: '{}'.", text),
      c.loc};
}
} // namespace pqxx


namespace pqxx::internal
{
constexpr char empty_composite_str[]{"()"};
} // namespace pqxx::internal


namespace pqxx
{
/// Estimate the buffer size needed to represent a value of a composite type.
/** Returns a conservative estimate.
 */
template<typename... T>
[[nodiscard]] inline std::size_t
composite_size_buffer(T const &...fields) noexcept
{
  constexpr auto num{sizeof...(fields)};

  // Size for a multi-field composite includes room for...
  //  + opening parenthesis
  //  + field budgets
  //  + separating comma per field
  //  - comma after final field
  //  + closing parenthesis
  //  + terminating zero

  if constexpr (num == 0)
    return std::size(pqxx::internal::empty_composite_str);
  else
    return 1 + (pqxx::internal::size_composite_field_buffer(fields) + ...) +
           num + 1;
}


/// Render a series of values as a single composite SQL value.
/** You may use this as a helper while implementing your own `string_traits`
 * for a composite type.
 *
 * @param loc An `std::source_location`, so that any error messages can report
 * this as the place where the error occurred.  This is probably more useful to
 * you than a location inside this function itself.
 *
 * After writing the composite's text representation to `buf`, this will append
 * a terminating zero.  This facilitates usage where the resulting SQL string
 * gets passed in as a query parameter.
 */
template<typename... T>
inline zview composite_into_buf(ctx c, std::span<char> buf, T const &...fields)
{
  if (std::size(buf) < composite_size_buffer(fields...))
    throw conversion_error{
      "Buffer space may not be enough to represent composite value.", c.loc};

  constexpr auto num_fields{sizeof...(fields)};
  if constexpr (num_fields == 0)
  {
    constexpr std::string_view empty{"()"};
    return zview{
      std::data(buf),
      pqxx::internal::copy_chars<true>(empty, buf, 0, c.loc) - 1};
  }

  std::size_t pos{0};
  // C++26: Use buf.at().
  buf[pos++] = '(';

  (pqxx::internal::write_composite_field<T>(buf, pos, fields, c), ...);

  // If we've got multiple fields, "backspace" that last comma.
  if constexpr (num_fields > 1)
    --pos;
  // C++26: Use buf.at().
  buf[pos++] = ')';
  buf[pos] = '\0';
  return zview{std::data(buf), pos};
}


/// Render a series of values as a single composite SQL value.
template<typename... T>
[[deprecated(
  "Pass conversion_context and std::span<char>, not two pointers.")]]
inline char *composite_into_buf(char *begin, char *end, T const &...fields)
{
  auto const text{composite_into_buf(
    {sl::current()}, std::span<char>{begin, end}, fields...)};
  return begin + std::size(text);
}
} // namespace pqxx
#endif
