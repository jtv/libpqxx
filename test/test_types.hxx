/* 
 * Custom types for testing & libpqxx support those types
 */

#include <pqxx/strconv>

#include <cstdint>
#include <cstring>
#include <exception>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string>
#include <vector>


class ipv4
{
public:
  ipv4() : m_as_int{0u} {}
  ipv4(const ipv4 &) =default;
  ipv4(ipv4 &&) =default;
  explicit ipv4(uint32_t i) : m_as_int{i} {}
  ipv4(
    unsigned char b1,
    unsigned char b2,
    unsigned char b3,
    unsigned char b4
  ) : ipv4()
  {
    set_byte(0, b1);
    set_byte(1, b2);
    set_byte(2, b3);
    set_byte(3, b4);
  }

  bool operator==(const ipv4 &o) const { return m_as_int == o.m_as_int; }
  ipv4 &operator=(const ipv4 &) =default;

  /// Index bytes, from 0 to 3, in network (i.e. Big-Endian) byte order.
  unsigned int operator[](int byte) const
  {
    if (byte < 0 or byte > 3)
        throw pqxx::usage_error("Byte out of range.");
    const auto shift = compute_shift(byte);
    return static_cast<unsigned int>((m_as_int >> shift) & 0xff);
  }

  /// Set individual byte, in network byte order.
  void set_byte(int byte, uint32_t value)
  {
    const auto shift = compute_shift(byte);
    const auto blanked = (m_as_int & ~uint32_t(0xff << shift));
    m_as_int = (blanked | ((value & 0xff) << shift));
  }

private:
  static unsigned compute_shift(int byte)
  {
    if (byte < 0 or byte > 3)
        throw pqxx::usage_error("Byte out of range.");
    return static_cast<unsigned>((3 - byte) * 8);
  }

  uint32_t m_as_int;
};


using bytea = std::vector<unsigned char>;


namespace pqxx
{
template<> struct nullness<ipv4> : no_null<ipv4> {};


template<> struct string_traits<ipv4>
{
  static inline constexpr int buffer_budget{16};

  static ipv4 from_string(std::string_view text)
  {
    ipv4 ts;
    if (text.data() == nullptr)
      internal::throw_null_conversion(type_name<ipv4>);
    std::regex ipv4_regex{
      "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})"
    };
    std::smatch match;
    // Need non-temporary for `std::regex_match()`
    std::string sstr{text};
    if (not std::regex_match(sstr, match, ipv4_regex) or match.size() != 5)
      throw std::runtime_error{"Invalid ipv4 format: " + std::string{text}};
    try
    {
      for (std::size_t i{0}; i < 4; ++i)
        ts.set_byte(int(i), uint32_t(std::stoi(match[i+1])));
    }
    catch (const std::invalid_argument&)
    {
      throw std::runtime_error{"Invalid ipv4 format: " + std::string{text}};
    }
    catch (const std::out_of_range&)
    {
      throw std::runtime_error{"Invalid ipv4 format: " + std::string{text}};
    }
    return ts;
  }

  static char *into_buf(char *begin, char *end, const ipv4 &value)
  {
    if (end - begin < buffer_budget)
      throw conversion_error{"Buffer too small for ipv4."};
    char *here = begin;
    for (int i=0; i < 4; ++i)
    {
      here = string_traits<unsigned>::into_buf(here, end, value[i]);
      *(here - 1) = '.';
    }
    *(here - 1) = '\0';
    return here;
  }

  static zview to_buf(char *begin, char *end, const ipv4 &value)
  {
    return zview{
	begin, static_cast<std::size_t>(into_buf(begin, end, value) - begin)};
  }

  static constexpr size_t size_buffer(const ipv4 &) noexcept
  { return buffer_budget; }
};


namespace
{
char nibble_to_hex(unsigned nibble)
{
  if (nibble < 10) return char('0' + nibble);
  else if (nibble < 16) return char('a' + (nibble - 10));
  else throw std::runtime_error{"Invalid digit going into bytea."};
}


unsigned hex_to_digit(char hex)
{
  auto x = static_cast<unsigned char>(hex);
  if (x >= '0' and x <= '9') return x - '0';
  else if (x >= 'a' and x <= 'f') return 10 + x - 'a';
  else if (x >= 'A' and x <= 'F') return 10 + x - 'A';
  else throw std::runtime_error{"Invalid hex in bytea."};
}
} // namespace


template<> struct nullness<bytea> : no_null<bytea> {};


template<> struct string_traits<bytea>
{
  static inline constexpr int buffer_budget{1000};

  static bytea from_string(std::string_view text)
  {
    if ((text.size() & 1) != 0) throw std::runtime_error{"Odd hex size."};
    bytea value;
    value.reserve((text.size() - 2) / 2);
    for (size_t i = 2; i < text.size(); i += 2)
    {
      auto hi = hex_to_digit(text[i]), lo = hex_to_digit(text[i + 1]);
      value.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return value;
  }

  static zview to_buf(char *begin, char *end, const bytea &value)
  {
    const auto need = size_buffer(value);
    const auto have = end - begin;
    if (std::size_t(have) < need)
      throw pqxx::conversion_overrun{"Not enough space in buffer for bytea."};
    char *pos = begin;
    *pos++ = '\\';
    *pos++ = 'x';
    for (const unsigned char u : value)
    {
      *pos++ = nibble_to_hex(unsigned(u) >> 4);
      *pos++ = nibble_to_hex(unsigned(u) & 0x0f);
    }
    *pos++ = '\0';
    return zview{begin, std::size_t(pos - begin - 1)};
  }

  static char *into_buf(char *begin, char *end, const bytea &value)
  { return begin + to_buf(begin, end, value).size() + 1; }

  static size_t size_buffer(const bytea &value)
  { return 2 * value.size() + 1; }
};
} // namespace pqxx
