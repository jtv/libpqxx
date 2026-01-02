/* Handling of SQL arrays.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/field instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
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

#include <algorithm>
#include <cassert>
#include <format>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "pqxx/connection.hxx"
#include "pqxx/encoding_group.hxx"

#include "pqxx/internal/array-composite.hxx"


namespace pqxx
{
// TODO: Specialise for string_view/zview, allocate all strings in one buffer.

/// An SQL array received from the database.
/** Parses an SQL array from its text format, making it available as a
 * container of C++-side values.
 *
 * The array can have one or more dimensions.  You must specify the number of
 * dimensions at compile time.  In each dimension, the array has a size which
 * the `array` constructor determines at run time based on the SQL array's
 * textual representation.  The sizes of a given SQL array are consistent: if
 * your array has two dimensions, for example, then it will have one
 * "horizontal" size which determines the number of elements in each row; and
 * it will have one "vertical" size which determines the number of rows.
 *
 * Physical memory storage is "row-major."  This means that the last of the
 * dimensions represents a row.  So in memory, element `a[m][n]` comes right
 * before `a[m][n+1]`.
 */
template<
  not_borrowed ELEMENT, std::size_t DIMENSIONS = 1u,
  char SEPARATOR = array_separator<ELEMENT>>
class array final
{
public:
  /// Parse an SQL array, read as text from a pqxx::result or stream.
  /** Uses `cx` only during construction, to find out the text encoding in
   * which it should interpret `data`.
   *
   * Once the `array` constructor completes, destroying or moving the
   * `connection` will not affect the `array` object in any way.
   *
   * @throws pqxx::unexpected_null if the array contains a null value, and the
   * `ELEMENT` type does not support null values.
   */
  array(std::string_view data, connection const &cx, sl loc = sl::current()) :
          array{data, cx.get_encoding_group(loc), loc}
  {}

  array(std::string_view data, encoding_group enc, sl loc = sl::current()) :
          m_ctx{enc, loc}
  {
    using group = encoding_group;
    switch (enc)
    {
    case group::unknown:
      throw usage_error{
        "Tried to parse array without knowing its encoding.", loc};

    case group::ascii_safe: parse<group::ascii_safe>(data, loc); break;
    case group::two_tier: parse<group::two_tier>(data, loc); break;
    case group::gb18030: parse<group::gb18030>(data, loc); break;
    case group::sjis: parse<group::sjis>(data, loc); break;
    default: PQXX_UNREACHABLE; break;
    }
  }

  /// The element type of values in this array
  using value_type = ELEMENT;

  /// How many dimensions does this array have?
  /** This value is known at compile time.
   */
  PQXX_PURE static constexpr std::size_t dimensions() noexcept
  {
    return DIMENSIONS;
  }

  /// What is the separator used for parsing this array's values?
  /** This value is known at compile time.
   */
  PQXX_PURE [[nodiscard]] static constexpr char separator() noexcept
  {
    return SEPARATOR;
  }

  /// Return the sizes of this array in each of its dimensions.
  /** The last of the sizes is the number of elements in a single row.  The
   * size before that is the number of rows of elements, and so on.  The first
   * is the "outer" size.
   */
  PQXX_PURE [[nodiscard]] std::array<std::size_t, DIMENSIONS> const &
  sizes() const noexcept
  {
    return m_extents;
  }

  template<std::integral... INDEX> ELEMENT const &at(INDEX... index) const
  {
    static_assert(sizeof...(index) == DIMENSIONS);
    check_bounds(index...);
    return m_elts.at(locate(index...));
  }

  /// Access element (without bounds check).
  /** Return element at given index.  Blindly assumes that the index lies
   * within the bounds of the array.  This is likely to be slightly faster than
   * `at()`.
   *
   * Multi-dimensional indexing using `operator[]` only works in C++23 or
   * better.  In older versions of C++ it will work only with
   * single-dimensional arrays.
   *
   * @warning This function is marked as "pure."  This means that if it fails,
   * depending on your compiler, the exception may occur in a different place
   * than you expected.  The compiler may even find scenarios where it can
   * avoid calling this operator, meaning that the exception does not happen at
   * all.  If you need more deterministic exception behaviour, use @ref at().
   */
  template<std::integral... INDEX>
  PQXX_PURE ELEMENT const &operator[](INDEX... index) const
  {
    static_assert(sizeof...(index) == DIMENSIONS);
    // TODO: Use operator[].  But Facebook's "infer" sees a buffer overflow.
    return m_elts.at(locate(index...));
  }

  /**
   * @name Array iteration
   *
   * There is no "nice" way to iterate over a multi-dimensional array as yet.
   * Instead, you can iterate over all the elements as if it were a simple,
   * flat, one-dimensional array.
   *
   * If this is a multi-dimensional array, iteration proceeds in row-major
   * order.  So for example, a two-dimensional array `a` would start at
   * `a[0, 0]`, then `a[0, 1]`, and so on.  Once it reaches the end of that
   * first row, it moves on to element `a[1, 0]`, and continues from there.
   */
  //@{
  /// Begin iteration of individual elements.
  PQXX_PURE constexpr auto cbegin() const noexcept { return m_elts.cbegin(); }
  /// Return end point of iteration.
  PQXX_PURE constexpr auto cend() const noexcept { return m_elts.cend(); }
  /// Begin iteration of individual elements.
  PQXX_PURE constexpr auto begin() const noexcept { return cbegin(); }
  /// Return endpoint of iteration.
  PQXX_PURE constexpr auto end() const noexcept { return cend(); }
  /// Begin reverse iteration.
  PQXX_PURE constexpr auto crbegin() const noexcept
  {
    return m_elts.crbegin();
  }
  /// Return end point of reverse iteration.
  PQXX_PURE constexpr auto crend() const noexcept { return m_elts.crend(); }
  PQXX_PURE constexpr auto rbegin() const noexcept { return crbegin(); }
  /// Return end point of reverse iteration.
  PQXX_PURE constexpr auto rend() const noexcept { return crend(); }
  //@}

  /// Number of elements in the array.
  /** This includes all elements, in all dimensions.  Therefore it is the
   * product of all values in `sizes()`.
   */
  constexpr std::size_t size() const noexcept { return m_elts.size(); }

  /// Number of elements in the array (as a signed number).
  /** This includes all elements, in all dimensions.  Therefore it is the
   * product of all values in `sizes()`.
   *
   * In principle, the size could get so large that it had no signed
   * equivalent.  If that can ever happen, this is your own problem and
   * behaviour is undefined.
   *
   * In practice however, I don't think `ssize()` could ever overflow.  You'd
   * need an array where each element takes up just one byte, such as Booleans,
   * filling up more than half your address space.  But the input string for
   * that array would need at least two bytes per value: one for the value, one
   * for the separating comma between elements.  So even then you wouldn't have
   * enough address space to create the array, even if your system allowed you
   * to use your full address space.
   */
  constexpr auto ssize() const noexcept
  {
    return static_cast<std::ptrdiff_t>(size());
  }

  /// Refer to the first element, if any.
  /** If the array is empty, dereferencing this results in undefined behaviour.
   */
  constexpr auto const &front() const noexcept { return m_elts.front(); }

  /// Refer to the last element, if any.
  /** If the array is empty, dereferencing this results in undefined behaviour.
   */
  constexpr auto const &back() const noexcept { return m_elts.back(); }

private:
  /// Throw an error if `data` is not a `DIMENSIONS`-dimensional SQL array.
  /** Sanity-checks two aspects of the array syntax: the opening braces at the
   * beginning, and the closing braces at the end.
   *
   * One syntax error this does not detect, for efficiency reasons, is for too
   * many closing braces at the end.  That's a tough one to detect without
   * walking through the entire array sequentially, and identifying all the
   * character boundaries.  The main parsing routine detects that one.
   */
  void check_dims(std::string_view data, sl loc)
  {
    auto sz{std::size(data)};
    if (sz < DIMENSIONS * 2)
      throw conversion_error{
        std::format(
          "Trying to parse a {}-dimensional array out of '{}'.", DIMENSIONS,
          data),
        loc};

    // Making some assumptions here:
    // * The array holds no extraneous whitespace.
    // * None of the sub-arrays can be null.
    // * Only ASCII characters start off with a byte in the 0-127 range.
    //
    // Given those, the input must start with a sequence of DIMENSIONS bytes
    // with the ASCII value for '{'; and likewise it must end with a sequence
    // of DIMENSIONS bytes with the ASCII value for '}'.

    if (data.at(0) != '{')
      throw conversion_error{"Malformed array: does not start with '{'.", loc};
    for (std::size_t i{0}; i < DIMENSIONS; ++i)
      if (data.at(i) != '{')
        throw conversion_error{
          std::format(
            "Expecting {}-dimensional array, but found {}.", DIMENSIONS, i),
          loc};
    if (data.at(DIMENSIONS) == '{')
      throw conversion_error{
        std::format(
          "Tried to parse {}-dimensional array from array data that has more "
          "dimensions.",
          DIMENSIONS),
        loc};
    for (std::size_t i{0}; i < DIMENSIONS; ++i)
      if (data.at(sz - 1 - i) != '}')
        throw conversion_error{
          "Malformed array: does not end in the right number of '}'.", loc};
  }

  /// Handle the end of a field.
  /** Check for a trailing separator, detect any syntax errors at this somewhat
   * complicated point, and return the offset where parsing should continue.
   */
  std::size_t
  parse_field_end(std::string_view data, std::size_t here, sl loc) const
  {
    auto const sz{std::size(data)};
    if (here < sz)
      switch (data.at(here))
      {
      case SEPARATOR:
        ++here;
        if (here >= sz)
          throw conversion_error{"Array looks truncated.", loc};
        switch (data.at(here))
        {
        case SEPARATOR:
          throw conversion_error{"Array contains double separator.", loc};
        case '}':
          throw conversion_error{"Array contains trailing separator.", loc};
        default: break;
        }
        break;
      case '}': break;
      default:
        throw conversion_error{
          std::format(
            "Unexpected character in array: {} where separator or closing "
            "brace expected.",
            static_cast<unsigned>(static_cast<unsigned char>(data.at(here)))),
          loc};
      }
    return here;
  }

  /// Estimate the number of elements in this array.
  /** We use this to pre-allocate internal storage, so that we don't need to
   * keep extending it on the fly.  It doesn't need to be too precise, so long
   * as it's fast; doesn't usually underestimate; and never overestimates by
   * orders of magnitude.
   */
  constexpr std::size_t estimate_elements(std::string_view data) const noexcept
  {
    // Dirty trick: just count the number of bytes that look as if they may be
    // separators.
    auto const separators{
      std::count(std::begin(data), std::end(data), SEPARATOR)};
    // The number of dimensions makes no difference here.  It's still one
    // separator between consecutive elements, just possibly with some extra
    // braces as well.
    return static_cast<std::size_t>(separators + 1);
  }

  template<encoding_group ENC> void parse(std::string_view data, sl loc)
  {
    static_assert(DIMENSIONS > 0u, "Can't create a zero-dimensional array.");
    conversion_context const c{m_ctx.enc, loc};
    auto const sz{std::size(data)};
    check_dims(data, loc);

    m_elts.reserve(estimate_elements(data));

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
    constexpr std::size_t outer{std::size_t{0u} - std::size_t{1u}};

    // We start parsing at the fictional outer dimension.  The input begins
    // with opening braces, one for each dimension, so we'll start off by
    // bumping all the way to the innermost dimension.
    std::size_t dim{outer};

    // Extent counters, one per "real" dimension.
    // Note initialiser syntax; this zero-initialises all elements.
    std::array<std::size_t, DIMENSIONS> extents{};

    // Current parsing position.
    std::size_t here{0};
    PQXX_ASSUME(here <= sz);
    while (here < sz)
    {
      if (data.at(here) == '{')
      {
        if (dim == outer)
        {
          // This must be the initial opening brace.
          if (know_extents_from != DIMENSIONS)
            throw conversion_error{
              "Array text representation closed and reopened its outside "
              "brace pair.",
              loc};
          assert(here == 0);
          PQXX_ASSUME(here == 0);
        }
        else
        {
          if (dim >= (DIMENSIONS - 1))
            throw conversion_error{
              "Array seems to have inconsistent number of dimensions.", loc};
          ++extents.at(dim);
        }
        // (Rolls over to zero if we're coming from the outer dimension.)
        ++dim;
        extents.at(dim) = 0u;
        ++here;
      }
      else if (data.at(here) == '}')
      {
        if (dim == outer)
          throw conversion_error{"Array has spurious '}'.", loc};
        if (dim < know_extents_from)
        {
          // We just finished parsing our first row in this dimension.
          // Now we know the array dimension's extent.
          m_extents.at(dim) = extents.at(dim);
          know_extents_from = dim;
        }
        else
        {
          if (extents.at(dim) != m_extents.at(dim))
            throw conversion_error{
              "Rows in array have inconsistent sizes.", loc};
        }
        // Bump back down to the next-lower dimension.  Which may be the outer
        // dimension, through underflow.
        --dim;
        ++here;
        here = parse_field_end(data, here, loc);
      }
      else
      {
        // Found an array element.  The actual elements always live in the
        // "inner" dimension.
        if (dim != DIMENSIONS - 1)
          throw conversion_error{
            "Malformed array: found element where sub-array was expected.",
            loc};
        assert(dim != outer);
        ++extents.at(dim);
        std::size_t end;
        switch (data.at(here))
        {
        case '\0':
          throw conversion_error{"Unexpected zero byte in array.", loc};
        case ',': throw conversion_error{"Array contains empty field.", loc};
        case '"': {
          // Double-quoted string.  We parse it into a buffer before parsing
          // the resulting string as an element.  This seems wasteful: the
          // string might not contain any special characters.  So it's
          // tempting to check, and try to use a string_view and avoid a
          // useless copy step.  But.  Even besides the branch prediction
          // risk, the very fact that the back-end chose to quote the string
          // indicates that there is some kind of special character in there.
          // So in practice, this optimisation would only apply if the only
          // special characters in the string were commas.
          end =
            pqxx::internal::scan_double_quoted_string<ENC>(data, here, loc);
          // TODO: scan_double_quoted_string() with reusable buffer.
          std::string const buf{
            pqxx::internal::parse_double_quoted_string<ENC>(
              data.substr(0, end), here, loc)};
          m_elts.emplace_back(from_string<ELEMENT>(buf, c));
        }
        break;
        default: {
          // Unquoted string.  An unquoted string is always literal, no
          // escaping or encoding, so we don't need to parse it into a
          // buffer.  We can just read it as a string_view.
          end = pqxx::internal::scan_unquoted_string<ENC, SEPARATOR, '}'>(
            data, here, loc);
          std::string_view const field{
            std::string_view{std::data(data) + here, end - here}};
          if (field == "NULL")
          {
            if constexpr (has_null<ELEMENT>())
              m_elts.emplace_back(make_null<ELEMENT>());
            else
              throw unexpected_null{
                std::format(
                  "Array contains a null {}.  Consider making it an array of "
                  "std::optional<{}> instead.",
                  name_type<ELEMENT>(), name_type<ELEMENT>()),
                loc};
          }
          else
            m_elts.emplace_back(from_string<ELEMENT>(field, c));
        }
        }
        here = end;
        PQXX_ASSUME(here <= sz);
        here = parse_field_end(data, here, loc);
      }
    }

    if (dim != outer)
      throw conversion_error{"Malformed array; may be truncated.", loc};
    assert(know_extents_from == 0);
    PQXX_ASSUME(know_extents_from == 0);

    init_factors();
  }

  /// Pre-compute indexing factors.
  void init_factors() noexcept
  {
    std::size_t factor{1};
    for (std::size_t dim{DIMENSIONS - 1}; dim > 0; --dim)
    {
      factor *= m_extents.at(dim);
      m_factors.at(dim - 1) = factor;
    }
  }

  /// Map a multidimensional index to an entry in our linear storage.
  template<typename... INDEX> std::size_t locate(INDEX... index) const noexcept
  {
    static_assert(
      sizeof...(index) == DIMENSIONS,
      "Indexing array with wrong number of dimensions.");
    return add_index(index...);
  }

  template<typename OUTER, typename... INDEX>
  constexpr std::size_t add_index(OUTER outer, INDEX... indexes) const noexcept
  {
    std::size_t const first{
      check_cast<std::size_t>(outer, "array index"sv, m_ctx.loc)};
    if constexpr (sizeof...(indexes) == 0)
    {
      return first;
    }
    else
    {
      static_assert(sizeof...(indexes) < DIMENSIONS);
      // (Offset by 1 here because the outer dimension is not in there.)
      constexpr auto dimension{DIMENSIONS - (sizeof...(indexes) + 1)};
      static_assert(dimension < DIMENSIONS);
      return first * m_factors.at(dimension) + add_index(indexes...);
    }
  }

  /// Check that indexes are within bounds.
  /** @throw pqxx::range_error if not.
   */
  template<typename OUTER, std::integral... INDEX>
  constexpr void check_bounds(OUTER outer, INDEX... indexes) const
  {
    std::size_t const first{
      check_cast<std::size_t>(outer, "array index"sv, m_ctx.loc)};
    static_assert(sizeof...(indexes) < DIMENSIONS);
    // (Offset by 1 here because the outer dimension is not in there.)
    constexpr auto dimension{DIMENSIONS - (sizeof...(indexes) + 1)};
    static_assert(dimension < DIMENSIONS);
    if (std::cmp_greater_equal(first, m_extents.at(dimension)))
      throw range_error{
        std::format(
          "Array index for dimension {} is out of bounds: {} >= {}.",
          dimension, first, m_extents.at(dimension)),
        m_ctx.loc};

    // Now check the rest of the indexes, if any.
    if constexpr (sizeof...(indexes) > 0)
      check_bounds(indexes...);
  }

  /// Linear storage for the array's elements.
  std::vector<ELEMENT> m_elts;

  /// Size along each dimension.
  std::array<std::size_t, DIMENSIONS> m_extents;

  /// Multiplication factors for indexing in each dimension.
  /** This wouldn't make any sense if `locate()` could recurse from the "inner"
   * dimension towards the "outer" one.  Unfortunately we've got to recurse in
   * the opposite direction, so it helps to pre-compute the factors.
   *
   * We don't need to cache a factor for the outer dimension, since we never
   * multiply by that number.
   */
  std::array<std::size_t, DIMENSIONS - 1> m_factors;

  /// Conversion context representing the construction point.
  /** It's not always possible to pass a context, e.g. in overloaded operators
   * or functions that take parameter packs (at least not nicely).  In those
   * situations, we use the construction point.
   */
  conversion_context m_ctx;
};


/// String traits for SQL arrays represented as @ref pqxx::array.
/** This supports two-way conversion.  There is also a more generic conversion
 * which only knows how to convert _to_ a string.
 */
template<typename ELEMENT, std::size_t DIMENSIONS>
struct string_traits<
  array<ELEMENT, DIMENSIONS, array_separator<ELEMENT>>> final
{
private:
  using elt_type = std::remove_cvref_t<ELEMENT>;
  using elt_traits = string_traits<elt_type>;
  static constexpr zview s_null{"NULL"};

public:
  using array_type = array<ELEMENT, DIMENSIONS, array_separator<elt_type>>;

  static std::string_view
  to_buf(std::span<char> buf, array_type const &value, ctx c = {})
  {
    auto const len{pqxx::internal::array_into_buf(buf, value, c)};
    assert(len > 0);
    assert(buf[len - 1] == '\0');
    return {std::data(buf), len - 1};
  }

  static std::size_t size_buffer(array_type const &value) noexcept
  {
    if constexpr (is_unquoted_safe<elt_type>)
      return 3 + std::accumulate(
                   std::begin(value), std::end(value), std::size_t{},
                   [](std::size_t acc, elt_type const &elt) {
                     // Budget for each element includes a terminating zero.
                     // We won't actually be wanting those, but don't subtract
                     // that one byte: we want room for a separator instead.
                     // However, std::size(s_null) doesn't account for the
                     // terminating zero, so add one to make s_null pay for its
                     // own separator.
                     return acc + (pqxx::is_null(elt) ?
                                     (std::size(s_null) + 1) :
                                     elt_traits::size_buffer(elt));
                   });
    else
      return 3 + std::accumulate(
                   std::begin(value), std::end(value), std::size_t{},
                   [](std::size_t acc, elt_type const &elt) {
                     // Opening and closing quotes, plus worst-case escaping,
                     // and the one byte for the trailing zero becomes room
                     // for a separator. However, std::size(s_null) doesn't
                     // account for the terminating zero, so add one to make
                     // s_null pay for its own separator.
                     std::size_t const elt_size{
                       pqxx::is_null(elt) ? (std::size(s_null) + 1) :
                                            elt_traits::size_buffer(elt)};
                     return acc + 2 * elt_size + 2;
                   });
  }

  static array_type from_string(std::string_view text, ctx c = {})
  {
    return array<ELEMENT, DIMENSIONS, array_separator<elt_type>>{
      text, c.enc, c.loc};
  }
};

/// No-null trait for SQL arrays represented as @ref pqxx::array.
template<typename ELEMENT, std::size_t DIMENSIONS>
struct nullness<array<ELEMENT, DIMENSIONS, array_separator<ELEMENT>>> final
        : no_null<array<ELEMENT, DIMENSIONS, array_separator<ELEMENT>>>
{};
} // namespace pqxx


namespace pqxx::internal
{
template<typename CONT>
concept containerlike = requires(CONT con) { CONT{con.begin(), con.end()}; };


template<typename CONT>
concept nonbinary_container = nonbinary_range<CONT> and containerlike<CONT>;
} // namespace pqxx::internal


namespace pqxx
{
/// A container of nonbinary data.  It has no inherent null value.
template<pqxx::internal::nonbinary_container CONT>
struct nullness<CONT> final : no_null<CONT>
{};


// TODO: Can we support multiple dimensions?
/// A container of nonbinary data can represent a 1-dimensional SQL array.
/** This does not define a conversion _to_ an SQL string; we leave that to the
 * `specialisation for nonbinary ranges.
 */
template<pqxx::internal::nonbinary_container CONT>
struct string_traits<CONT> final : pqxx::internal::nonbinary_range_traits<CONT>
{
  [[nodiscard]] static CONT from_string(std::string_view text, ctx c = {})
  {
    using value_type = typename CONT::value_type;
    using array_type = array<value_type>;
    auto const arr = pqxx::from_string<array_type>(text, c);
    return CONT{arr.cbegin(), arr.cend()};
  }
};


/// Low-level parser for C++ arrays.  @deprecated Use @ref pqxx::array instead.
/** Clunky old API for parsing SQL arrays.
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
class PQXX_LIBEXPORT array_parser final
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
  [[deprecated("Use pqxx::array instead.")]]
  explicit array_parser(
    std::string_view input, encoding_group = encoding_group::ascii_safe);

  /// Parse the next step in the array.
  /** Returns what it found.  If the juncture is @ref juncture::string_value,
   * the string will contain the value.  Otherwise, it will be empty.
   *
   * Call this until the @ref array_parser::juncture it returns is
   * @ref juncture::done.
   */
  std::pair<juncture, std::string> get_next(sl loc = sl::current())
  {
    return (this->*m_impl)(loc);
  }

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
  using implementation =
    std::pair<juncture, std::string> (array_parser::*)(sl);

  /// Pick the `implementation` for `enc`.
  static implementation specialize_for_encoding(encoding_group enc, sl loc);

  /// Our implementation of `parse_array_step`, specialised for our encoding.
  implementation m_impl;

  /// Perform one step of array parsing.
  template<encoding_group>
  std::pair<juncture, std::string> parse_array_step(sl loc);

  template<encoding_group> std::size_t scan_double_quoted_string(sl loc) const;
  template<encoding_group>
  std::string parse_double_quoted_string(std::size_t end, sl loc) const;
  template<encoding_group> std::size_t scan_unquoted_string(sl loc) const;
  template<encoding_group>
  std::string_view parse_unquoted_string(std::size_t end, sl loc) const;
};
} // namespace pqxx
#endif
