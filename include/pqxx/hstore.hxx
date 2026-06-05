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


/// Scan an unquoted hstore string.
/** Set `IS_KEY` to `true` when scanning a key string, or to `false` when
 * scanning a value string.
 */
template<encoding_group ENC, bool IS_KEY>
inline constexpr std::size_t
scan_unquoted_hstore_string(std::string_view input, std::size_t pos, sl loc)
{
  // This is where unquoted strings in hstore differ from unquoted strings in
  // arrays or composite types.  Any un-escaped whitespace will terminate the
  // string.
  if constexpr (IS_KEY)
  {
    // In a key, an unescaped equals sign ('=') will end the string.
    return scan_unquoted_string<
      ENC, ',', ' ', '\f', '\t', '\n', '\r', '\v', '='>(input, pos, loc);
  }
  else
  {
    return scan_unquoted_string<ENC, ',', ' ', '\f', '\t', '\n', '\r', '\v'>(
      input, pos, loc);
  }
}
} // namespace pqxx::internal


namespace pqxx
{
/// An iterator for an @ref hstore_parse.
/** This class does all the heavy lifting, so these are not lightweight
 * objects.  They are not trivial to create, copy, or move.
 */
template<typename KEY, typename VALUE> class hstore_iterator final
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
  /// Find the extent of the key or value string at `offset` in `m_input`.
  /** Set `IS_KEY` to `true` when parsing a key, and to `false` when parsing a
   * value.  There's a difference.  :-/
   */
  template<encoding_group ENC, bool IS_KEY>
  [[nodiscard]] std::size_t scan_string(std::size_t offset) const
  {
    if ((std::size(m_input) > offset) and (m_input.at(offset) == '"'))
      return pqxx::internal::scan_double_quoted_string<ENC, '\\'>(
        m_input, offset, m_ctx.loc);
    else
      return pqxx::internal::scan_unquoted_hstore_string<ENC, IS_KEY>(
        m_input, offset, m_ctx.loc);
  }

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

    // XXX: Backend parses "1=>,2" as "1" => ",2".

    // Scan the key string.
    here = scan_string<ENC, true>(here);

    auto const key_input = m_input.substr(m_offset, here - m_offset);

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
    here = scan_string<ENC, false>(here);

    // The value can be null, but we can detect that later, during operator*(),
    // since m_value includes any quotes around the value.  So we'll be able to
    // distinguish `NULL` from `"NULL"` at that time.
    auto const value_input = m_input.substr(value_start, here - value_start);

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

    // Parse key & value into m_buffer, and update m_key/m_value accordingly.
    // We're sizing m_buffer pessimistically.  The parsed key and value strings
    // will be able to fit in this space.
    m_buffer.resize(std::size(key_input) + std::size(value_input));
    auto const sep{parse_string<ENC>(key_input, 0u)};
    m_key = std::string_view{std::data(m_buffer), sep};
    m_value = std::string_view{
      std::data(m_buffer) + sep, parse_string<ENC>(value_input, sep) - sep};
  }

  /// Move ahead in the input buffer to the next entry.
  /** Updates `m_offset` to point to the next entry (if any), and sets `m_key`
   * and `m_value` to refer to the current entry's parsed key and value
   * strings, respectively.
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
    // XXX: No!  Should check the unparsed string!
    return (m_value.size() == 4) and
           ((m_value[0] == 'N') or (m_value[0] == 'n')) and
           ((m_value[1] == 'U') or (m_value[1] == 'u')) and
           ((m_value[2] == 'L') or (m_value[2] == 'l')) and
           ((m_value[3] == 'L') or (m_value[3] == 'l'));
  }

  /// Parse a non-null hstore string, whether key or value, into `m_buffer`.
  /** Unquotes and unescapes as needed.
   *
   * @param input Input string: either key or value string.
   * @param offset Starting offset of free space in `m_buffer`.
   * @return One-past-end offset of data written into `m_buffer`.
   */
  template<encoding_group ENC>
  [[nodiscard]] std::size_t
  parse_string(std::string_view input, std::size_t offset)
  {
    auto const buffer{std::span<char>{m_buffer}.subspan(offset)};
    if ((std::size(input) > 0u) and (input.at(0) == '"'))
    {
      return offset + pqxx::internal::parse_double_quoted_string<ENC, '\\'>(
                        buffer, input, 0, m_ctx.loc);
    }
    else
    {
      return offset + pqxx::internal::parse_unquoted_string<ENC>(
                        buffer, input, 0, m_ctx.loc);
    }
  }

  /// Parse a non-null hstore string, whether key or value.
  /** Unquotes and unescapes as needed.
   *
   * This version takes its encoding at run time.
   */
  [[nodiscard]] std::size_t
  parse_string(std::string_view input, std::size_t offset)
  {
    // Dynamic-to-static switch.
    switch (m_ctx.enc)
    {
    case encoding_group::unknown:
      throw conversion_error{
        "Can't parse hstore data: no encoding set.", m_ctx.loc};
    case encoding_group::ascii_safe:
      return parse_string<encoding_group::ascii_safe>(input, offset);
    case encoding_group::two_tier:
      return parse_string<encoding_group::two_tier>(input, offset);
    case encoding_group::gb18030:
      return parse_string<encoding_group::gb18030>(input, offset);
    case encoding_group::sjis:
      return parse_string<encoding_group::sjis>(input, offset);
    default: PQXX_UNREACHABLE;
    }
  }

  /// Extract and return entry's key.
  [[nodiscard]] KEY get_key() const { return from_string<KEY>(m_key, m_ctx); }

  /// Extract and return entry's value.
  [[nodiscard]] VALUE get_value() const
  {
    // Awkwardly phrased in hopes of getting mandatory return value
    // optimization.
    if constexpr (has_null<VALUE>())
    {
      return is_null() ? make_null<VALUE>() :
                         from_string<VALUE>(m_value, m_ctx);
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
      return from_string<VALUE>(m_value, m_ctx);
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
