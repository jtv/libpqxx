/* 
 * Custom types for testing & libpqxx support those types
 */

#include <pqxx/strconv>

#include <cstdint>
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
  ) :
    m_as_int{uint32_t(b1) << 24 | uint32_t(b2) << 16 | uint32_t(b3) << 8 | b4}
  {}

  bool operator==(const ipv4 &o) const { return m_as_int == o.m_as_int; }
  ipv4 &operator=(const ipv4 &) =default;

  /// Index bytes, from 0 to 3, in network (i.e. Big-Endian) byte order.
  unsigned int operator[](int byte) const
  {
    if (byte < 0 or byte > 3)
        throw pqxx::usage_error("Byte out of range.");
    const auto shift = compute_shift(byte);
    return static_cast<unsigned char>((m_as_int >> shift) & 0xff);
  }

  /// Set individual byte, in network byte order.
  void set_byte(int byte, uint32_t value)
  {
    const auto shift = unsigned(compute_shift(byte));
    const auto blanked = m_as_int & ~uint32_t(0xff << shift);
    m_as_int = blanked | ((value & 0xff) << shift);
  }

private:
  static uint32_t compute_shift(int byte)
  {
    if (byte < 0 or byte > 3)
        throw pqxx::usage_error("Byte out of range.");
    return uint32_t((3 - byte) * 8);
  }

  uint32_t m_as_int;
};


using bytea = std::vector<unsigned char>;


template<typename T> class custom_optional
{
  // Very basic, only has enough features for testing
private:
  union
  {
    // "Blank" member just so no T object needs to be constructed here.
    void *_;
    T value;
  };
  bool has_value;
public:
  custom_optional() : has_value{false} {}
  custom_optional(std::nullptr_t) : has_value{false} {}
  custom_optional(const custom_optional &o) : has_value{o.has_value}
  {
    if (has_value) new(&value) T(o.value);
  }
  custom_optional(const T& v) : value{v}, has_value{true} {}
  ~custom_optional()
  {
    if (has_value) value.~T();
  }
  explicit operator bool() const { return has_value; }
  T &operator *()
  {
    if (has_value) return value;
    else throw std::logic_error{"bad optional access"};
  }
  const T &operator *() const
  {
    if (has_value) return value;
    else throw std::logic_error{"bad optional access"};
  }
  custom_optional &operator =(const custom_optional &o)
  {
    if (&o == this) return *this;
    if (has_value and o.has_value)
      value = o.value;
    else
    {
      if (has_value) value.~T();
      if (o.has_value) new(&value) T(o.value);
    }
    has_value = o.has_value;
    return *this;
  }
  custom_optional &operator =(std::nullptr_t)
  {
    if (has_value) value.~T();
    has_value = false;
    return *this;
  }
};


namespace pqxx
{
template<> struct string_traits<ipv4>
{
  using subject_type = ipv4;

  static constexpr const char* name() noexcept
  {
    return "ipv4";
  }

  static constexpr bool has_null() noexcept { return false; }

  static bool is_null(const subject_type &) { return false; }

  [[noreturn]] static subject_type null()
  {
    internal::throw_null_conversion(name());
  }

  static void from_string(const char str[], subject_type &ts)
  {
    if (str == nullptr) internal::throw_null_conversion(name());
    std::regex ipv4_regex{
      "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})"
    };
    std::smatch match;
    // Need non-temporary for `std::regex_match()`
    std::string sstr{str};
    if (not std::regex_match(sstr, match, ipv4_regex) or match.size() != 5)
      throw std::runtime_error{
        "invalid ipv4 format: " + std::string{str}
      };
    try
    {
      for (std::size_t i{0}; i < 4; ++i)
        ts.set_byte(int(i), uint32_t(std::stoi(match[i+1])));
    }
    catch (const std::invalid_argument&)
    {
      throw std::runtime_error{
        "invalid ipv4 format: " + std::string{str}
      };
    }
    catch (const std::out_of_range&)
    {
      throw std::runtime_error{
        "invalid ipv4 format: " + std::string{str}
      };
    }
  }

  static std::string to_string(const subject_type &ts)
  {
    return
      std::to_string(ts[0])
      + "."
      + std::to_string(ts[1])
      + "."
      + std::to_string(ts[2])
      + "."
      + std::to_string(ts[3])
    ;
  }
};
} // namespace pqxx


namespace pqxx
{
template<> struct string_traits<bytea>
{
private:
  static unsigned char from_hex(char c)
  {
    if (c >= '0' and c <= '9')
      return static_cast<unsigned char>(c - '0');
    else if (c >= 'a' and c <= 'f')
      return static_cast<unsigned char>(c - 'a' + 10);
    else if (c >= 'A' and c <= 'F')
      return static_cast<unsigned char>(c - 'A' + 10);
    else
      throw std::range_error{
        "Not a hexadecimal digit: " + std::string{c} +
        " (value " + std::to_string(static_cast<unsigned int>(c)) + ")."
      };
  }
  static unsigned char from_hex(char c1, char c2)
  {
    return static_cast<unsigned char>(
        (from_hex(c1) << 4) | (from_hex(c2) & 0x0f)
    );
  }

public:
  using subject_type = bytea;

  static constexpr const char* name() noexcept
  {
    return "bytea";
  }

  static constexpr bool has_null() noexcept { return false; }

  static bool is_null(const subject_type &) { return false; }

  [[noreturn]] static subject_type null()
  {
    internal::throw_null_conversion( name() );
  }

  static void from_string(const char str[], subject_type& bs)
  {
    if (str == nullptr)
      internal::throw_null_conversion(name());
    auto len = strlen(str);
    if (len % 2 or len < 2 or str[0] != '\\' or str[1] != 'x')
      throw std::runtime_error{
        "invalid bytea format: " + std::string{str}
      };
    bs.clear();
    for (std::size_t i{2}; i < len; /**/)
    {
      bs.emplace_back(from_hex(str[i], str[i + 1]));
      i += 2;
    }
  }

  static std::string to_string(const subject_type &bs)
  {
    std::stringstream s;
    s << "\\x" << std::hex;
    for (auto b : bs)
      s
        << std::setw(2)
        << std::setfill('0')
        << static_cast<unsigned int>(b)
      ;
    return s.str();
  }
};
} // namespace pqxx
