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

#include <cassert>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "pqxx/internal/encoding_group.hxx"
#include "pqxx/internal/encodings.hxx"


namespace pqxx
{
/// An SQL array received from the database.
template<
  typename ELEMENT, std::size_t DIMENSIONS = 1,
  char SEPARATOR = array_separator<ELEMENT>>
class array final
{
public:
  /// How many dimensions does this array have?
  constexpr std::size_t dimensions() noexcept { return DIMENSIONS; }

  std::array<std::size_t, DIMENSIONS> const &sizes() noexcept
  {
    return m_extents;
  }

  template<typename... INDEX> ELEMENT const &at(INDEX...... index) const
  {
    // XXX: Check bounds on all dimensions.
    return m_elts.at(locate(index...));
  }

  /// Access element (without bounds check).
  /** Return element at given index.  Blindly assumes that the index lies within
   * the bounds of the array.
   *
   * Multi-dimensional indexing using `operator[]` only works in C++23 or
   * better.  In older versions of C++ it will only work with single-dimensional
   * arrays.
   */
  template<typename... INDEX> ELEMENT const &operator[](INDEX... index) const
  {
    return m_elts[locate(index...)];
  }

private:
  explicit array(std::string_view data, pqxx::internal::encoding_group enc)
  {
    static_assert(DIMENSIONS > 0, "Can't create a zero-dimensional array.");
    auto constexpr sz{std::size(data)};
    if (sz < DIMENSIONS * 2)
      throw conversion_error{pqxx::internal::concat(
        "Trying to parse a ", DIMENSIONS, "-dimensional array out of '", data,
        "'.")};

    // Making some assumptions here:
    // * The array holds no extraneous whitespace.
    // * None of the sub-arrays can be null.
    // * Only ASCII characters start off with a byte in the 0-127 range.
    //
    // Given those, the input must start with a sequence of DIMENSIONS bytes
    // with the ASCII value for '{'; and likewise it must end with a sequence
    // of DIMENSIONS bytes with the ASCII value for '}'.

    if (data[0] != '{')
      throw conversion_error{"Malformed array: does not start with '{'."};
    for (std::size_t i{0}; i < DIMENSIONS; ++i)
      if (data[i] != '{')
        throw conversion_error{concat(
          "Expecting ", DIMENSIONS, "-dimensional array, but found ", i, ".")};
    if (std::data[DIMENSIONS] == '{')
      throw conversion_error{concat(
        "Tried to parse ", DIMENSIONS,
        "-dimensional array from array data that has more dimensions.")};
    for (std::size_t i{0}; i < DIMENSIONS; ++i)
      if (data[sz - 1 - i] != '}')
        throw conversion_error{
          "Malformed array: does not end in the right number of '}'."};

    // We discover the array's extents along each of the dimensions, starting
    // with the final dimension and working our way towards the first.  At any
    // given point during parsing, we know the extents starting at this
    // dimension.
    std::size_t know_extents_from{DIMENSIONS};

    // Currently parsing this dimension.  We start off at -1, relying on C++'s
    // well-defined rollover for unsigned numbers.
    // The actual outermost dimension of the array is 0, and the innermost is
    // at the end.  But, the array as a whole is enclosed in braces just like
    // each row.  So we act like there's an anomalous "outer" dimension holding
    // the entire array.
    constexpr std::size_t outer{0u - 1u};

    // We start parsing at the fictional outer dimension.  The input begins
    // with opening braces, one for each dimension, so we'll start off by
    // bumping all the way to the innermost dimension.
    std::size_t dim{outer};

    // Extent counters, one per "real" dimension.
    // Note initialiser syntax; this should zero-initialise all elements.
    std::array<std::size_t, DIMENSIONS> extents{};
    for (auto const e : extents) assert(e == 0u);

    // Current parsing position.
    std::size_t here{0};
    while (here < sz)
    {
      if (input[here] == '{')
      {
        if (dim == outer)
        {
          // This must be the initial opening brace.
          if (know_extents_from != DIMENSIONS)
            throw conversion_error{
              "Array text representation closed and reopened its outside "
              "brace pair."};
          assert(here == 0);
        }
        else
        {
          if (dim >= (DIMENSIONS - 1))
            throw conversion_error{
              "Array seems to have inconsistent number of dimensions."};
          ++extents[dim];
        }
        ++dim;
        extents[dim] = 0u;
        ++here;
      }
      else if (input[here] == '}')
      {
        if (dim == outer)
          throw conversion_error{"Array has spurious '}'."};
        if (dim < know_extents_from)
        {
          // We just finished parsing our first row in this dimension.
          // Now we know the array dimension's extent.
          m_extents[dim] = extents[dim];
          know_extents_from = dim;
        }
        else
        {
          if (extents[dim] != m_extents[dim])
            throw conversion_error{"Rows in array have inconsistent sizes."};
        }
        // Bump back down to the next-lower dimension.
        --dim;
      }
      else
      {
        // Found an array element.  The actual elements always live in the
        // "inner" dimension.
        if (dim != DIMENSIONS - 1)
          throw conversion_error{
            "Malformed array: found element where sub-array was expected."};
        ++extents[dim];
        // XXX: * Scan for SEPARATOR.
        // XXX: * Parse element.
        // XXX: * m_elts.emplace_back(parse(...));
      }
    }

    if (dim != outer)
      throw conversion_error{"Malformed array; may be truncated."};
    assert(know_extents_from == 0);
    if (extents[0] != 1)
      throw conversion_error{"Malformed array: multiple arrays in one."};
  }

  /// Map a multidimensional index to an entry in our linear storage.
  template<typename... INDEX> std::size_t locate(std::size_t... index) const
  {
    static_assert(
      sizeof...(index) == DIMENSIONS,
      "Indexing array with wrong number of dimensions.");
    static_assert(std::is_convertible<INDEX, std::size_t> and ...);
    // XXX: return index[-1] + m_extents[-1] * (index[-2] + m_extents[-2] *
    // (index[-3] + ...))
  }

  /// Linear storage for the array's elements.
  std::vector<ELEMENT> m_elts;

  /// Size along each dimension.
  std::array<std::size_t, DIMENSIONS> m_extents;
};


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
