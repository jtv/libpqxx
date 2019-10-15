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


/// Estimate how much buffer space we need to render a T object.
/** Ignore the parameter.  It's just there to help us disable inappropriate
 * specialisations for the given type.
 */
template<typename T> constexpr
std::enable_if_t<
	(
		std::is_arithmetic_v<T> and
		std::numeric_limits<T>::digits10 > 0
	),
	int
>
size_buffer(T *)
{
  // Allocate room for how many digits?  There's "max_digits10" for
  // floating-point numbers, but only "digits10" for integer types.
  // They're really different: digits10 is the maximum number of decimal
  // digits that you can reliably fit into the type, but max_digits10 is the
  // maximum number that you can get out.  The latter is what we want here,
  // but we don't get it for integral types, where it's digits10 + 1.
  // Leave a little bit of extra room for sign, decimal point, and trailing
  // null byte.
  return 3 + std::max({
	std::numeric_limits<T>::digits10 + 1,
	std::numeric_limits<T>::max_digits10});
}


template<typename T> constexpr
std::enable_if_t<std::is_enum_v<T>, int>
size_buffer(T *)
{ return size_buffer(static_cast<std::underlying_type_t<T> *>(nullptr)); }


// No buffer space needed for nullptr_t: it's always just a null.
constexpr int size_buffer(std::nullptr_t *) { return 0; }
// No buffer space needed for bool: we use fixed strings for true/false.
constexpr int size_buffer(bool *) { return 0; }


template<typename T> constexpr int size_buffer(std::optional<T> *)
{ return size_buffer(static_cast<T *>(nullptr)); }
template<typename T> constexpr int size_buffer(std::unique_ptr<T> *)
{ return size_buffer(static_cast<T *>(nullptr)); }
template<typename T> constexpr int size_buffer(std::shared_ptr<T> *)
{ return size_buffer(static_cast<T *>(nullptr)); }
} // namespace pqxx::internal

namespace pqxx
{
// TODO: Move into string traits.
/// How big of a buffer do we want for representing a T object as text?
/** Specialisations may be 0 to indicate that they don't need any buffer
 * space at all.  This would be the case for types where all strings are fixed:
 * @c bool and @c nullptr_t.  In such cases, @c to_buf must never dereference
 * its @c begin * and @c end arguments at all.
 */
template<typename T>
inline constexpr int buffer_budget =
	pqxx::internal::size_buffer(static_cast<T *>(nullptr));
} // namespace pqxx


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


template<typename T> PQXX_LIBEXPORT extern
zview to_buf_integral(char *, char *, T);
template<typename T> PQXX_LIBEXPORT extern
std::string_view to_buf_float(char *, char *, T);
template<typename T> PQXX_LIBEXPORT extern
std::string to_string_float(T);
} // namespace pqxx::internal


namespace pqxx::internal
{
bool PQXX_LIBEXPORT from_string_bool(std::string_view);
template<typename T> T PQXX_LIBEXPORT from_string_integral(std::string_view);
template<typename T> T PQXX_LIBEXPORT from_string_float(std::string_view);


/// String traits for builtin integral types (though not bool).
template<typename T>
struct integral_traits
{
  static T from_string(std::string_view text)
  { return from_string_integral<T>(text); }
  static zview to_buf(char *begin, char *end, const T &value)
  { return to_buf_integral(begin, end, value); }
};


/// String traits for builtin floating-point types.
template<typename T>
struct float_traits
{
  static T from_string(std::string_view text)
  { return from_string_float<T>(text); }
  static zview to_buf(char *begin, char *end, const T &value)
  { return to_buf_float(begin, end, value); }
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
  static bool from_string(std::string_view text)
  { return internal::from_string_bool(text); }

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
  static constexpr zview to_buf(
	char *, char *, const std::nullptr_t &
  ) noexcept
  { return zview{}; }
};


template<typename T> inline zview
to_buf(char *begin, char *end, const std::optional<T> &value)
{
  if (value.has_value()) return to_buf(begin, end, *value);
  else return zview{};
}


template<typename T> inline zview
to_buf(char *begin, char *end, const std::shared_ptr<T> &value)
{
  if (value) return to_buf(begin, end, *value);
  else return zview{};
}


template<typename T> inline zview
to_buf(char *begin, char *end, const std::unique_ptr<T> &value)
{
  if (value) return to_buf(begin, end, *value);
  else return zview{};
}
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


template<> struct string_traits<const std::string>
{
  // No conversions implemented.
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


/// A @c std::unique_ptr is a bit like a @c std::optional.
template<typename T> struct string_traits<std::unique_ptr<T>>
{
  static std::unique_ptr<T> from_string(std::string_view text)
  { return std::make_unique<T>(string_traits<T>::from_string(text)); }
};


template<typename T> struct nullness<std::shared_ptr<T>>
{
  static constexpr bool has_null = true;
  static constexpr bool is_null(const std::shared_ptr<T> &t) noexcept
	{ return not t or pqxx::is_null(*t); }
  static constexpr std::shared_ptr<T> null() { return std::shared_ptr<T>{}; }
};


/// A @c std::shared_ptr is a bit like a @c std::optional.
template<typename T> struct string_traits<std::shared_ptr<T>>
{
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
  std::array<char, buffer_budget<T>> m_buf;
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
