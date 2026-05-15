/* Hstore support for libpqxx.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/hstore intead.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_HSTORE_HXX
#define PQXX_HSTORE_HXX

namespace pqxx::internal
{
/// The `end()` of a @ref hstore_parse.
PQXX_LIBEXPORT class hstore_end final
{};
} // namespace pqxx::internal


namespace pqxx
{
/// An iterator for an @ref hstore_parse.
/** This class does all the heavy lifting, so these are not lightweight
 * objects.  They are not trivial to create, copy, or move.
 */
template<typename KEY, typename VALUE>
PQXX_LIBEXPORT class hstore_iterator final
{
  hstore_iterator(std::string_view input, ctx c) : m_input{input}, m_ctx{c} {}
  hstore_iterator() = delete;
  hstore_iterator(hstore_iterator const &) = default;
  hstore_iterator(hstore_iterator &&) = default;
  hstore_iterator &operator=(hstore_iterator const &) = default;
  hstore_iterator &operator=(hstore_iterator &&) = default;

  [[nodiscard]] std::pair<KEY, VALUE> operator*() const
  {
    return {
      from_string<KEY>(m_key, m_ctx), from_string<VALUE>(m_value, m_ctx)};
  }

  hstore_iterator &operator++()
  {
    auto const sz{std::size(m_input)};
    std::size_t here{pqxx::internal::skip_ascii_whitespace(m_input, m_offset)};

    // XXX: Scan key string.
    m_key = m_input.substr(m_offset, here - m_offset);

    here = pqxx::internal::skip_ascii_whitespace(m_input, here);

    // Scan "=>".
    if ((here + 2) < sz)
      throw pqxx::conversion_error{
        std::format("Truncated hstore value: '{}'", m_input)};
    if ((input.at(here) != '=') or (input.at(here + 1) != '>'))
      throw pqxx::conversion_error{
        std::format(
          "Expected '=>' in hstore at offset {}: '{}'", here, m_input),
        m_ctx};
    here += 2;

    here = pqxx::internal::skip_ascii_whitespace(m_input, here);
    auto const value_start{here};
    // XXX: Scan value string.
    // XXX: This can be NULL (unquoted)... how do we represent that?
    m_value = m_input.substr(value_start, here - value_start);

    here = pqxx::internal::skip_ascii_whitespace(m_input, here);

    // If there's a trailing comma, position us right behind there so we can
    // parse the next step.
    //
    // (There may be whistespace after the comma, but there's no point scanning
    // that yet; the next invocation will have to scan leading whitespace
    // anyway.)
    //
    // We don't care about encoding here.  In every supported encoding, any
    // character starting with an ASCII-range byte is a single-byte ASCII
    // character.
    if ((here < std::size(m_input)) and (m_input.at(here) == ','))
      ++here;

    m_offset = here;

    return *this;
  }

  /// Post-increment operator.  Prefer pre-increment.
  [[nodiscard]] hstore_iterator operator++(int)
  {
    hstore_iterator old{*this};
    ++*this;
    return old;
  }

  int operator<=>(hstore_end const &) const noexcept
  {
    return m_offset <=> std::size(m_input);
  }

private:
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
PQXX_LIBEXPORT class hstore_parse final
{
public:
  /// Prepare to parse an hstore.
  /** The text to which `input` points MUST remain valid and unchanged for the
   * duration of the parse.  The parse ends when the last iterator dereference
   * has completed (i.e. typically when your iteration hits `end()`).
   */
  hstore_parse(std::string_view input, ctx c = {}) :
          m_input{input, c}, m_ctx{c}
  {}

  hstore_parse() = default;
  hstore_parse(hstore_parse const &) = default;
  hstore_parse(hstore_parse &&) = default;
  hstore_parse &operator=(hstore_parse const &) = default;
  hstore_parse &operator=(hstore_parse &&) = default;

  [[nodiscard]] hstore_iterator<KEY, VALUE> cbegin() const noexcept
  {
    return hstore_iterator < KEY, VALUE{m_input};
  }

  [[nodiscard]] auto begin() const noexcept { return cbegin(); }

  [[nodiscard]] hstore_end cend() const noexcept { return {}; }
  [[nodiscard]] auto end() const noexcept { return cend(); }

private:
  /// The input string.
  std::string_view m_input;

  conversion_context m_ctx;
};
} // namespace pqxx

#endif
