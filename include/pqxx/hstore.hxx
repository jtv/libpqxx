/* Hstore support for libpqxx.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/hstore instead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_HSTORE_HXX
#define PQXX_HSTORE_HXX

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include "pqxx/internal/strings.hxx"
#include "pqxx/strconv.hxx"

namespace pqxx::internal
{
/// The `end()` of a @ref hstore_parse.
struct hstore_end final
{
  [[nodiscard]] auto operator<=>(hstore_end const &) const noexcept
  {
    return 0;
  }

  /// For comparison to @ref hstore_iterator (not yet defined here).
  template<typename RHS>
  [[nodiscard]] auto operator==(RHS const &r) const noexcept
  {
    return r == *this;
  }

  /// For comparison to @ref hstore_iterator (not yet defined here).
  template<typename RHS>
  [[nodiscard]] auto operator!=(RHS const &r) const noexcept
  {
    return r != *this;
  }
};


template<encoding_group ENC>
inline constexpr std::size_t
scan_unquoted_hstore_string(std::string_view input, std::size_t pos, sl loc)
{
  // This is where unquoted strings in hstore differ from unquoted strings in
  // arrays or composite types.  Any un-escaped whitespace will terminate the
  // string.
  // XXX: Check for escapes.
  // XXX: Parameterise for the various terminating characters.
  return find_ascii_char<ENC, ',', ' ', '\f', '\t', '\n', '\r', '\v'>(
    input, pos, loc);
}
} // namespace pqxx::internal


namespace pqxx
{
/// An iterator for an @ref hstore_parse.
/** This class does all the heavy lifting, so these are not lightweight
 * objects.  They are not trivial to create, copy, or move.
 */
template<typename KEY, typename VALUE>
class hstore_iterator final
{
public:
  hstore_iterator(std::string_view input, ctx c) :
          m_input{input},
          m_ctx{c},
          m_offset{pqxx::internal::skip_ascii_whitespace(input, 0u)}
  {
    // As an invariant for scan_entry(), we must move past any leading
    // whitespace.  Hence the skip_ascii_whitespace() in the initialisers.
    // This is also how we can detect end(): m_offset will point to the end of
    // m_input.
    scan_entry();
  }
  hstore_iterator() = delete;
  hstore_iterator(hstore_iterator const &) = default;
  hstore_iterator(hstore_iterator &&) = default;
  ~hstore_iterator() = default;
  hstore_iterator &operator=(hstore_iterator const &) = default;
  hstore_iterator &operator=(hstore_iterator &&) = default;

  [[nodiscard]] std::pair<KEY, VALUE> operator*() const
  {
    // XXX: Parse into m_buffer!
    // XXX: Dynamic-to-static switch here so we only switch once.
    return {get_key(), get_value()};
  }

  /// Move ahead to the next item in the hstore.
  /** If there is a next item, caches the locations of the key and value fields
   * in the input view into `m_key` and `m_value`, for use by the next call to
   * `operator*()`..
   */
  hstore_iterator &operator++()
  {
    scan_entry();
    return *this;
  }

  /// Post-increment operator.  Prefer pre-increment.
  [[nodiscard]] hstore_iterator operator++(int)
  {
    hstore_iterator old{*this};
    ++*this;
    return old;
  }

  [[nodiscard]] auto operator==(pqxx::internal::hstore_end) const noexcept
  {
    return m_offset == std::size(m_input);
  }
  [[nodiscard]] auto operator!=(pqxx::internal::hstore_end e) const noexcept
  {
    return not(*this == e);
  }

  [[nodiscard]] auto operator<=>(hstore_iterator const &rhs) const noexcept
  {
    return m_offset <=> rhs.m_offset;
  }

private:
  /// Move ahead in the input buffer to the next entry.
  /** Updates `m_offset`.  Caches the locations of the key and value fields in
   * `m_key` and `m_value` respectively.
   *
   * This version of the call takes the encoding group as a template parameter.
   * There is also a version that figures this out at run time.
   *
   * As an invariant, before and after this call, `m_offset` always points at
   * either the end of `m_input`, or at the beginning of the entry we're about
   * to advance beyond.  This also means that there must never be any
   * whitespace at `m_offset`.
   */
  template<pqxx::encoding_group ENC> void scan_entry()
  {
    auto const sz{std::size(m_input)};
    std::size_t here{m_offset};

    // If we're at the end, we're done.
    if (here >= sz)
      return;

    // There is no further whitespace at m_offset.
    assert(pqxx::internal::skip_ascii_whitespace(m_input, here) == here);

    if (m_input.at(here) == '"')
      here = pqxx::internal::scan_double_quoted_string<ENC, '\\'>(
        m_input, here, m_ctx.loc);
    else
      here = pqxx::internal::scan_unquoted_hstore_string<ENC>(
        m_input, here, m_ctx.loc);

    m_key = m_input.substr(m_offset, here - m_offset);

    here = pqxx::internal::skip_ascii_whitespace(m_input, here);

    // Scan "=>".
    if (m_input.substr(here, 2u) != "=>")
      throw pqxx::conversion_error{
        std::format(
          "Expected '=>' in hstore at offset {}: '{}'", here, m_input),
        m_ctx.loc};
    here += 2;

    here = pqxx::internal::skip_ascii_whitespace(m_input, here);
    if (here >= sz)
      throw pqxx::conversion_error{
        std::format("No value in hstore entry: '{}'", m_input), m_ctx.loc};

    auto const value_start{here};
    if (m_input.at(here) == '"')
      here = pqxx::internal::scan_double_quoted_string<ENC, '\\'>(
        m_input, here, m_ctx.loc);
    else
      here = pqxx::internal::scan_unquoted_hstore_string<ENC>(
        m_input, here, m_ctx.loc);

    // The value can be null, but we can detect that later, during operator*(),
    // since m_value includes any quotes around the value.  So we'll be able to
    // distinguish `NULL` from `"NULL"` at that time.
    m_value = m_input.substr(value_start, here - value_start);

    here = pqxx::internal::skip_ascii_whitespace(m_input, here);

    // If there's a trailing comma, position us right behind it so we can parse
    // the next step.
    //
    // (There may be whitespace after the comma, but there's no point scanning
    // that yet; the next invocation will have to scan leading whitespace
    // anyway.)
    //
    // We don't care about encoding here.  In every supported encoding, any
    // character starting with an ASCII-range byte is a single-byte ASCII
    // character.
    if ((here < std::size(m_input)) and (m_input.at(here) == ','))
    {
      ++here;
      here = pqxx::internal::skip_ascii_whitespace(m_input, here);
    }

    m_offset = here;
  }

  /// Move ahead in the input buffer to the next entry.
  /** Updates `m_offset` to point to the next entry (if any), and sets `m_key`
   * and `m_value` to refer to the current entry's key and value strings,
   * respectively.
   *
   * This version resolves the encoding group at run time.  There is also a
   * version of this function that takes the encoding group as a template
   * parameter.
   */
  void scan_entry()
  {
    // Dynamic-to-static switch.
    switch (m_ctx.enc)
    {
    case encoding_group::unknown:
      throw conversion_error{
        "Can't parse hstore data: no encoding set.", m_ctx.loc};
    case encoding_group::ascii_safe:
      scan_entry<encoding_group::ascii_safe>();
      break;
    case encoding_group::two_tier:
      scan_entry<encoding_group::two_tier>();
      break;
    case encoding_group::gb18030: scan_entry<encoding_group::gb18030>(); break;
    case encoding_group::sjis: scan_entry<encoding_group::sjis>(); break;
    }
  }

  /// Does this entry have a null value?
  [[nodiscard]] bool is_null() const noexcept
  {
    return (m_value.size() == 4) and
           ((m_value[0] == 'N') or (m_value[0] == 'n')) and
           ((m_value[1] == 'U') or (m_value[1] == 'u')) and
           ((m_value[2] == 'L') or (m_value[2] == 'l')) and
           ((m_value[3] == 'L') or (m_value[3] == 'l'));
  }

  /// Parse a non-null hstore string, whether key or value.
  /** Unquotes and unescapes as needed.
   */
  template<typename T, encoding_group ENC>
  [[nodiscard]] T parse_string(std::string_view input) const
  {
    if ((std::size(input) >= 2u) and (input.at(0) == '"'))
    {
      return from_string<T>(
        pqxx::internal::parse_double_quoted_string<ENC, '"'>(
          input, 0, m_ctx.loc),
        m_ctx);
    }
    else
    {
      return from_string<T>(
        pqxx::internal::parse_unquoted_string<ENC>(m_input, 0, m_ctx.loc),
        m_ctx);
    }
  }

  /// Parse a non-null hstore string, whether key or value.
  /** Unquotes and unescapes as needed.
   *
   * This version takes its encoding at run time.
   */
  template<typename T>
  [[nodiscard]] T parse_string(std::string_view input) const
  {
    // Dynamic-to-static switch.
    switch (m_ctx.enc)
    {
    case encoding_group::unknown:
      throw conversion_error{
        "Can't parse hstore data: no encoding set.", m_ctx.loc};
    case encoding_group::ascii_safe:
      return parse_string<T, encoding_group::ascii_safe>(input);
    case encoding_group::two_tier:
      return parse_string<T, encoding_group::two_tier>(input);
    case encoding_group::gb18030:
      return parse_string<T, encoding_group::gb18030>(input);
    case encoding_group::sjis:
      return parse_string<T, encoding_group::sjis>(input);
    default: PQXX_UNREACHABLE;
    }
  }

  /// Extract and return entry's key.
  [[nodiscard]] KEY get_key() const { return parse_string<KEY>(m_key); }

  /// Extract and return entry's value.
  [[nodiscard]] VALUE get_value() const
  {
    // Awkwardly phrased in hopes of getting mandatory return value
    // optimization.
    if constexpr (has_null<VALUE>())
    {
      return is_null() ? make_null<VALUE>() : parse_string<VALUE>(m_value);
    }
    else
    {
      // Arguably this should be a pqxx::unexpected_null, but that gets a bit
      // weird when the target type does support nulls.  It's the hstore entry
      // that's invalid, not the conversion per se.
      if (is_null())
        throw conversion_error{
          std::format(
            "Tried to read null hstore value with key '{}' as a {}, which "
            "does not support nulls.",
            m_key, name_type<VALUE>()),
          m_ctx.loc};
      return parse_string<VALUE>(m_value);
    }
  }

  /// Hstore input string.
  std::string_view m_input;

  /// Conversion context.
  /** Keeps track of such things as the text encoding, and the client's call
   * site for error reporting.
   */
  conversion_context m_ctx;

  /// Offset of the next hstore item in `m_input`.
  std::size_t m_offset{0u};

  /// The key and value strings as found in `m_input`.
  std::string_view m_key, m_value;

  /// Reusable buffer for parsing an entry.
  /** Stores the data for the current key and value.  If the key and value
   * being parsed are `std::string_view`s, they will refer to this buffer.
   *
   * The buffer becomes invalid when parsing of the next entry begins.
   */
  std::vector<char> m_buffer;
};


/// A parsing pass of an hstore string.
/** Helps you parse the [hstore](
 *   https://www.postgresql.org/docs/current/hstore.html) text format.
 *
 * Iterating this parses the hstore's keys and values as `KEY` and `VALUE`
 * values, respectively.
 */
template<typename KEY = std::string_view, typename VALUE = std::string_view>
class hstore_parse final
{
public:
  /// Prepare to parse an hstore.
  /** The text to which `input` points MUST remain valid and unchanged for the
   * duration of the parse.  The parse ends when the last iterator dereference
   * has completed (i.e. typically when your iteration hits `end()`).
   */
  explicit hstore_parse(std::string_view input, ctx c = {}) :
          m_input{input}, m_ctx{c}
  {}

  hstore_parse() = delete;
  hstore_parse(hstore_parse const &) = default;
  hstore_parse(hstore_parse &&) = default;
  ~hstore_parse() = default;
  hstore_parse &operator=(hstore_parse const &) = default;
  hstore_parse &operator=(hstore_parse &&) = default;

  [[nodiscard]] auto cbegin() const
  {
    return hstore_iterator<KEY, VALUE>{m_input, m_ctx};
  }

  [[nodiscard]] auto begin() const { return cbegin(); }

  [[nodiscard]] auto cend() const noexcept
  {
    return pqxx::internal::hstore_end{};
  }
  [[nodiscard]] auto end() const noexcept { return cend(); }

private:
  /// The input string.
  std::string_view m_input;

  conversion_context m_ctx;
};
} // namespace pqxx

#endif
