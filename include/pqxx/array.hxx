/* Handling of SQL arrays.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/field instead.
 *
 * Copyright (c) 2000-2022, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ARRAY
#define PQXX_H_ARRAY

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include <stdexcept>
#include <string>
#include <utility>

#include "pqxx/internal/encoding_group.hxx"
#include "pqxx/internal/encodings.hxx"


namespace pqxx
{
/// Low-level array parser.
/** @warning This is not a great API.  Something nicer is on the way.
 *
 * Use this to read an array field retrieved from the database.
 *
 * @warning This parser will only work reliably if your client encoding is
 * UTF-8, ASCII, or a "safe ASCII superset" (such as the EUC encodings) where
 * a byte value in the ASCII range can only occur as an actual ASCII character,
 * never as one byte in a multi-byte character.
 *
 * @warning The parser only supports array element types which use a comma
 * (`','`) as the separator between array elements.  All built-in SQL types use
 * comma, except for `box` which uses semicolon.  However some custom types may
 * not work.
 *
 * The input is a C-style string containing the textual representation of an
 * array, as returned by the database.  The parser reads this representation
 * on the fly.  The string must remain in memory until parsing is done.
 *
 * Parse the array by making calls to @ref get_next until it returns a
 * @ref juncture of `done`.  The @ref juncture tells you what the parser found
 * in that step: did the array "nest" to a deeper level, or "un-nest" back up?
 */
class PQXX_LIBEXPORT array_parser
{
public:
  /// What's the latest thing found in the array?
  enum class juncture
  {
    /// Starting a new row.
    row_start,
    /// Ending the current row.
    row_end,
    /// Found a NULL value.
    null_value,
    /// Found a string value.
    string_value,
    /// Parsing has completed.
    done,
  };

  /// Constructor.  You don't need this; use @ref field::as_array instead.
  /** The parser only remains valid while the data underlying the @ref result
   * remains valid.  Once all `result` objects referring to that data have been
   * destroyed, the parser will no longer refer to valid memory.
   */
  explicit array_parser(
    std::string_view input,
    internal::encoding_group = internal::encoding_group::MONOBYTE);

  /// Parse the next step in the array.
  /** Returns what it found.  If the juncture is @ref juncture::string_value,
   * the string will contain the value.  Otherwise, it will be empty.
   *
   * Call this until the @ref array_parser::juncture it returns is
   * @ref juncture::done.
   */
  std::pair<juncture, std::string> get_next() { return (this->*m_impl)(); }

private:
  std::string_view m_input;

  /// Current parsing position in the input.
  std::size_t m_pos = 0u;

  /// A function implementing the guts of `get_next`.
  /** Internally this class uses a template to specialise the implementation of
   * `get_next` for each of the various encoding groups.  This allows the
   * compiler to inline the parsing of each text encoding, which happens in
   * very hot loops.
   */
  using implementation = std::pair<juncture, std::string> (array_parser::*)();

  /// Pick the `implementation` for `enc`.
  static implementation
  specialize_for_encoding(pqxx::internal::encoding_group enc);

  /// Our implementation of `parse_array_step`, specialised for our encoding.
  implementation m_impl;

  /// Perform one step of array parsing.
  template<pqxx::internal::encoding_group>
  std::pair<juncture, std::string> parse_array_step();

  template<pqxx::internal::encoding_group>
  std::string::size_type scan_double_quoted_string() const;
  template<pqxx::internal::encoding_group>
  std::string parse_double_quoted_string(std::string::size_type end) const;
  template<pqxx::internal::encoding_group>
  std::string::size_type scan_unquoted_string() const;
  template<pqxx::internal::encoding_group>
  std::string parse_unquoted_string(std::string::size_type end) const;

  template<pqxx::internal::encoding_group>
  std::string::size_type scan_glyph(std::string::size_type pos) const;
  template<pqxx::internal::encoding_group>
  std::string::size_type
  scan_glyph(std::string::size_type pos, std::string::size_type end) const;
};
} // namespace pqxx
#endif
