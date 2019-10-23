#include <array>
#include <map>
#include <memory>
#include <optional>
#include <type_traits>
#include <vector>


namespace pqxx::internal
{
/// Convert a number in [0, 9] to its ASCII digit.
inline constexpr char number_to_digit(int i) noexcept
{ return static_cast<char>(i+'0'); }
} // namespace pqxx::internal


/* Internal helpers for string conversion.
 *
 * Do not include this header directly.  The libpqxx headers do it for you.
 */
namespace pqxx::internal
{
/// Summarize buffer overrun.
std::string PQXX_LIBEXPORT state_buffer_overrun(
	ptrdiff_t have_bytes,
	ptrdiff_t need_bytes);


/// Throw exception for attempt to convert null to given type.
[[noreturn]] PQXX_LIBEXPORT void throw_null_conversion(
	const std::string &type);


template<typename T> PQXX_LIBEXPORT extern std::string to_string_float(T);
} // namespace pqxx::internal


namespace pqxx::internal
{
/// String traits for builtin integral types (though not bool).
template<typename T>
struct integral_traits
{
  /// Maximum buffer size needed to represent this type.
  /** Includes a sign if needed; the number of base-10 digits which the type;
   * can reliably represent; the one extra base-10 digit which the type can
   * only partially represent; and the terminating zero.
   */
  static inline constexpr int buffer_budget{
	std::is_signed_v<T> + std::numeric_limits<T>::digits10 + 1 + 1};

  static PQXX_LIBEXPORT T from_string(std::string_view text);
  static PQXX_LIBEXPORT zview to_buf(char *begin, char *end, const T &value);
};


/// String traits for builtin floating-point types.
template<typename T>
struct float_traits
{
  /// Maximum buffer size needed to represent this type.
  /** Includes a sign if needed; a possible leading zero before the decimal
   * point; the full number of base-10 digits which may be needed; a decimal
   * point if needed; and the terminating zero.
   */
  static inline constexpr int buffer_budget =
	1 + 1 + std::numeric_limits<T>::max_digits10 + 1 + 1;

  static PQXX_LIBEXPORT T from_string(std::string_view text);
  static PQXX_LIBEXPORT zview to_buf(char *begin, char *end, const T &value);
};
} // namespace pqxx::internal


namespace pqxx
{
/// The built-in arithmetic types do not have inherent null values.
template<typename T>
struct nullness<T, std::enable_if_t<std::is_arithmetic_v<T>>> : no_null<T>
{
};


template<> struct string_traits<short> :
	internal::integral_traits<short> {};
template<> struct string_traits<unsigned short> :
	internal::integral_traits<unsigned short> {};
template<> struct string_traits<int> :
	internal::integral_traits<int> {};
template<> struct string_traits<unsigned> :
	internal::integral_traits<unsigned> {};
template<> struct string_traits<long> :
	internal::integral_traits<long> {};
template<> struct string_traits<unsigned long> :
	internal::integral_traits<unsigned long> {};
template<> struct string_traits<long long> :
	internal::integral_traits<long long> {};
template<> struct string_traits<unsigned long long> :
	internal::integral_traits<unsigned long long> {};
template<> struct string_traits<float> :
	internal::float_traits<float> {};
template<> struct string_traits<double> :
	internal::float_traits<double> {};
template<> struct string_traits<long double> :
	internal::float_traits<long double> {};


template<> struct string_traits<bool>
{
  static inline constexpr int buffer_budget = 0;

  static PQXX_LIBEXPORT bool from_string(std::string_view text);

  static constexpr zview to_buf(char *, char *, const bool &value) noexcept
  { return value ? "true" : "false"; }
};


template<typename T> struct nullness<std::optional<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::optional<T> &v) noexcept
	{ return ((not v.has_value()) or pqxx::is_null(*v)); }
  static constexpr std::optional<T> null() { return std::optional<T>{}; }
};


template<typename T> struct string_traits<std::optional<T>>
{
  static inline constexpr int buffer_budget = string_traits<T>::buffer_budget;

  zview to_buf(char *begin, char *end, const std::optional<T> &value)
  {
    if (value.has_value()) return to_buf(begin, end, *value);
    else return zview{};
  }

  static std::optional<T> from_string(std::string_view text)
  {
    return std::optional<T>{
	std::in_place, string_traits<T>::from_string(text)};
  }
};


template<typename T>
inline T from_string(const std::stringstream &text)			//[t00]
	{ return from_string<T>(text.str()); }


template<> struct string_traits<std::nullptr_t>
{
  static inline constexpr int buffer_budget = 0;

  static constexpr zview to_buf(
	char *, char *, const std::nullptr_t &
  ) noexcept
  { return zview{}; }
};
} // namespace pqxx


namespace pqxx
{
template<> struct nullness<const char *>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const char *t) noexcept
	{ return t == nullptr; }
  static constexpr const char *null() noexcept { return nullptr; }
};


/// String traits for C-style string ("pointer to const char").
template<> struct string_traits<const char *>
{
  static const char *from_string(std::string_view text) { return text.data(); }

  static zview to_buf(char *begin, char *end, const char * const &value)
  {
    if (value == nullptr) return zview{};
    auto len = std::strlen(value);
    auto buf_size = end - begin;
    if (buf_size < ptrdiff_t(len)) throw conversion_overrun{
	"Could not copy string: buffer too small.  " +
        pqxx::internal::state_buffer_overrun(buf_size, ptrdiff_t(len))
	};
    std::memcpy(begin, value, len + 1);
    return zview{begin, len};
  }
};


template<> struct nullness<char *>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const char *t) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }
};


/// String traits for non-const C-style string ("pointer to char").
template<> struct string_traits<char *>
{
  static zview to_buf(char *begin, char *end, char * const &value)
  { return string_traits<const char *>::to_buf(begin, end, value); }

  // Don't allow conversion to this type since it breaks const-safety.
};


template<size_t N> struct nullness<char[N]> : no_null<char[N]> {};


/// String traits for C-style string constant ("array of char").
template<size_t N> struct string_traits<char[N]>
{
  // Don't allow conversion to this type since it breaks const-safety.
};


template<> struct nullness<std::string> : no_null<std::string> {};


template<> struct string_traits<std::string>
{
  static std::string from_string(std::string_view text)
	{ return std::string{text}; }

  static zview to_buf(char *begin, char *end, const std::string &value)
  {
    if (value.size() >= std::size_t(end - begin))
      throw conversion_overrun{
  	"Could not convert string to string: too long for buffer."};
    std::memcpy(begin, value.c_str(), value.size() + 1);
    return zview{begin, value.size()};
  }
};


/// There's no real null for @c std::string_view.
/** I'm not sure how clear-cut this is: a @c string_view may have a null
 * data pointer, which is analogous to a null @c char pointer.
 */
template<> struct nullness<std::string_view> : no_null<std::string_view> {};


/// String traits for `string_view`.
template<> struct string_traits<std::string_view>
{
  // Don't allow conversion to this type; it has nowhere to store its contents.
};


template<> struct nullness<zview> : no_null<zview> {};


/// String traits for `zview`.
template<> struct string_traits<zview>
{
  // Don't allow conversion to this type; it has nowhere to store its contents.
};


template<> struct nullness<std::stringstream> : no_null<std::stringstream> {};


template<> struct string_traits<std::stringstream>
{
  static std::stringstream from_string(std::string_view text)
  {
    std::stringstream stream;
    stream.write(text.data(), std::streamsize(text.size()));
    return stream;
  }
};


template<> struct nullness<std::nullptr_t>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(std::nullptr_t) noexcept { return true; }
  static constexpr std::nullptr_t null() { return nullptr; }
};


template<typename T> struct nullness<std::unique_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::unique_ptr<T> &t) noexcept
	{ return not t or pqxx::is_null(*t); }
  static constexpr std::unique_ptr<T> null()
	{ return std::unique_ptr<T>{}; }
};


template<typename T> struct string_traits<std::unique_ptr<T>>
{
  static inline constexpr int buffer_budget = string_traits<T>::buffer_budget;

  static std::unique_ptr<T> from_string(std::string_view text)
  { return std::make_unique<T>(string_traits<T>::from_string(text)); }

  zview to_buf(char *begin, char *end, const std::unique_ptr<T> &value)
  {
    if (value) return to_buf(begin, end, *value);
    else return zview{};
  }
};


template<typename T> struct nullness<std::shared_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::shared_ptr<T> &t) noexcept
	{ return not t or pqxx::is_null(*t); }
  static constexpr std::shared_ptr<T> null() { return std::shared_ptr<T>{}; }
};


template<typename T> struct string_traits<std::shared_ptr<T>>
{
  static inline constexpr int buffer_budget = string_traits<T>::buffer_budget;

  static std::shared_ptr<T> from_string(std::string_view text)
  { return std::make_shared<T>(string_traits<T>::from_string(text)); }
};
} // namespace pqxx


namespace pqxx::internal
{
/// Implementation of @c str for types for which we have @c to_buf.
/** The second template parameter is only there to support conditional
 * specialisation.  We only take the address of @c to_buf to sabotage
 * inappropriate specialisations.
 */
template<
	typename T,
	typename = std::enable_if_t<&string_traits<T>::to_buf != nullptr>
>
class str_impl
{
public:
  explicit str_impl(const T &value) :
    m_view(
      string_traits<T>::to_buf(
        m_buf.data(), m_buf.data() + m_buf.size(), value)
    )
  {}

  constexpr zview view() const noexcept { return m_view; }
  constexpr const char *c_str() const noexcept { return m_view.data(); }

private:
  std::array<char, string_traits<T>::buffer_budget> m_buf;
  zview m_view;
};


template<> class str_impl<std::string, void>
{
public:
  explicit str_impl(const std::string &value) : m_str{value} {}
  explicit str_impl(std::string &&value) : m_str{std::move(value)} {}

  zview view() const noexcept { return zview(m_str); }
  const char *c_str() const noexcept { return m_str.c_str(); }

private:
  std::string m_str;
};


/// Inline char arrays up to this size inside a @c str.
constexpr std::size_t string_stack_threshold = 100;


/// Store a small char array.  @warn This assumes a terminating zero!
template<std::size_t N>
class str_impl<char[N], std::enable_if_t<(N < string_stack_threshold)>>
{
public:
  explicit str_impl(const char (&text)[N]) noexcept
  { std::memcpy(m_buf, text, N); }

  zview view() const noexcept
  {
    // This is where we assume a terminating zero.  The string_view is one byte
    // less than the size of the array, even though the c_str() may well be the
    // full length of the array.
    return zview{m_buf, N - 1};
  }
  const char *c_str() const noexcept { return m_buf; }

private:
  char m_buf[N];
};


/// Store a large char array.
/** The version of str_impl for larger char arrays stores its data on the heap,
 * to avoid a single large string using up all of the thread's stack space.
 *
 * @warn This assumes a terminating zero!
 */
template<std::size_t N>
class str_impl<char[N], std::enable_if_t<(N >= string_stack_threshold)>>
{
public:
  explicit str_impl(const char (&text)[N]) noexcept : m_str{text} {}

  zview view() const noexcept { return zview(m_str); }
  const char *c_str() const noexcept { return m_str.c_str(); }

private:
  std::string m_str;
};


template<> class str_impl<const char *, void>
{
public:
  explicit str_impl(const char value[]) : m_str{value} {}

  zview view() const noexcept { return zview(m_str); }
  const char *c_str() const noexcept { return m_str.c_str(); }

private:
  std::string m_str;
};


template<> class str_impl<char *, void> : public str<const char *>
{
public:
  explicit str_impl(const char value[]) : str<const char *>{value}
  {}
};


#if !defined(PQXX_HAVE_CHARCONV_FLOAT)
/// Optimised @c str for environments without floating-point charconv.
/** If the language implementation does not provide charconv for floating-point
 * types, we convert floating-point values to strings using a costly little
 * dance around @c std::stringstream, which naturally produces a
 * @c std::string, not something more lightweight like @c string_view.
 *
 * The real drama is that the @c to_buf implementation in this situation is
 * more expensive than converting to @c std::string.  So, we have a dedicated
 * @c str implementation built on top of a @c string.
 */
template<> class str_impl<float, void>
{
public:
  explicit str_impl(float value) : m_str{internal::to_string_float(value)} {}

  zview view() const noexcept { return zview(m_str); }
  const char *c_str() const noexcept { return m_str.c_str(); }

private:
  std::string m_str;
};
#endif


template<typename T> class str_impl<std::optional<T>, void> : public str<T>
{
public:
  explicit str_impl(const std::optional<T> &value) : str<T>{*value} {}
};


template<typename T> class str_impl<std::unique_ptr<T>, void> : public str<T>
{
public:
  explicit str_impl(const std::unique_ptr<T> &value) : str<T>{*value} {}
};


template<typename T> class str_impl<std::shared_ptr<T>, void> : public str<T>
{
public:
  explicit str_impl(const std::shared_ptr<T> &value) : str<T>{*value} {}
};
} // namespace pqxx::internal


namespace pqxx
{
template<typename T> inline std::string to_string(const T &value)
{
  if (is_null(value))
    throw conversion_error{
	"Attempt to convert null " + type_name<T> + " to a string."};

  pqxx::str<T> text{value};
  return std::string{text.view()};
}


template<> inline std::string to_string(const float &value)
	{ return internal::to_string_float(value); }
template<> inline std::string to_string(const double &value)
	{ return internal::to_string_float(value); }
template<> inline std::string to_string(const long double &value)
	{ return internal::to_string_float(value); }
template<> inline std::string to_string(const std::stringstream &value)
{ return value.str(); }
} // namespace pqxx
