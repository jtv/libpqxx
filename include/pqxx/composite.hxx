#include "pqxx/internal/array-composite.hxx"
#include "pqxx/util.hxx"

// TODO: Also support converting a C++ value to an SQL composite type.

namespace pqxx
{
/// Parse a string representation of a value of a composite type.
/** @warning This code is still experimental.  Use with care.
 *
 * Interprets @c text as the string representation of a value of some
 * composite type, and sets each of @c fields to the respective values of its
 * fields.  The field types must be copy-assignable.
 *
 * The number of fields must match the number of fields in the composite type,
 * and there must not be any other text in the input.  The function is meant to
 * handle any value string that the backend can produce, but not necessarily
 * every valid alternative spelling.
 *
 * Fields in composite types can be null.  When this happens, the C++ type of
 * the corresponding field reference must be of a type that can handle nulls.
 * If you are working with a type that does not have an inherent null value,
 * such as e.g. @c int, consider using @c std::optional.
 */
template<typename... T>
inline void parse_composite(
  pqxx::internal::encoding_group enc, std::string_view text, T &... fields)
{
  static_assert(sizeof...(fields) > 0);

  auto const scan{pqxx::internal::get_glyph_scanner(enc)};
  auto const data{text.data()};
  auto const size{text.size()};
  if (size == 0)
    throw conversion_error{"Cannot parse composite value from empty string."};

  std::size_t here{0}, next{scan(data, size, here)};
  if (next != 1 or data[here] != '(')
    throw conversion_error{
      "Invalid composite value string: " + std::string{data}};

  here = next;

  constexpr auto num_fields{sizeof...(fields)};
  std::size_t index{0};
  (pqxx::internal::parse_composite_field(
     index, text, here, fields, scan, num_fields - 1),
   ...);
}


/// Parse a string representation of a value of a composite type.
/** @warning This version only works for UTF-8 and single-byte encodings.
 *
 * For proper encoding support, pass an @c encoding_group as the first
 * argument.
 */
template<typename... T>
inline void parse_composite(std::string_view text, T &... fields)
{
  parse_composite(pqxx::internal::encoding_group::MONOBYTE, text, fields...);
}
} // namespace pqxx
