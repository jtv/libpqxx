#include "pqxx/internal/array-composite.hxx"
#include "pqxx/util.hxx"

namespace pqxx
{
/// Parse a string representation of a value of a composite type.
/**
 */
template<typename... T>
inline void parse_composite(
  std::string_view text, T... &fields,
  pqxx::internal::encoding_group enc =
    pqxx::internal::encoding_group::MONOBYTE)
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
  auto const last_field{sizeof...(fields) - 1};
  (parse_composite_field<T, indexes>(text, here, fields, num_fields), ...);
}
} // namespace pqxx
