/* Internal helpers for string conversion.
 *
 * Do not include this header directly.  The libpqxx headers do it for you.
 */
namespace pqxx::internal
{
/// Throw exception for attempt to convert null to given type.
[[noreturn]] PQXX_LIBEXPORT void throw_null_conversion(
	const std::string &type);


constexpr char number_to_digit(int i) noexcept
	{ return static_cast<char>(i+'0'); }

/// How big of a buffer do we want for representing a TYPE object as text?
template<typename TYPE> [[maybe_unused]] constexpr int size_buffer() noexcept
{
  using lim = std::numeric_limits<TYPE>;
  // Allocate room for how many digits?  There's "max_digits10" for
  // floating-point numbers, but only "digits10" for integer types.
  constexpr auto digits = std::max({lim::digits10, lim::max_digits10});
  // Leave a little bit of extra room for signs, decimal points, and the like.
  return digits + 4;
}


#if __has_include(<charconv>)
/// Formulate an error message for a failed std::to_chars call.
inline std::string make_conversion_error(
	std::to_chars_result res, const std::string &type)
{
  std::string msg;
  switch (res.ec)
  {
  case std::errc::value_too_large:
    msg = ": Value too large for buffer.";
    break;
  default:
    msg = ".";
    break;
  }

  return std::string{"Could not convert "} + type + " to string" + msg;
}
#endif


/// Write nonnegative integral value at end of buffer.  Return start.
/** Includes a single trailing null byte, right before @c *end.
 */
template<typename T> inline char *
nonneg_to_buf(char *begin, char *end, T value)
{
  char *pos = end;
  *--pos = '\0';
  do
  {
    *--pos = pqxx::internal::number_to_digit(int(value % 10));
    value /= 10;
  } while (value > 0);
  return pos;
}


/// A few hard-coded string versions of difficult negative numbers.
/** For template argument n, this gives the string for n-1.
 * The reason is that compilers won't accept as a template argument negative
 * number that doesn't have an absolute value, as is the case on
 * std::numeric_limits<long long>::min() in a two's-complement system.
 */
template<auto MIN> constexpr const char *hard_neg;
template<> constexpr const char *hard_neg<-126> = "-127";
template<> constexpr const char *hard_neg<-127> = "-128";
template<> constexpr const char *hard_neg<-32766> = "-32767";
template<> constexpr const char *hard_neg<-32767> = "-32768";
template<> constexpr const char *hard_neg<-2147483646> = "-2147483647";
template<> constexpr const char *hard_neg<-2147483647> = "-2147483648";
template<> constexpr const char *hard_neg<-9223372036854775806> =
	"-9223372036854775807";
template<> constexpr const char *hard_neg<-9223372036854775807> =
	"-9223372036854775808";


/// Represent an integral value as a string_view.  May use given buffer.
template<typename T> inline std::string_view
to_buf_integral(char *begin, char *end, T value)
{
  // TODO: Conservative estimate.  We could do better, but will it cost time?
  if (end - begin < size_buffer<T>())
    throw conversion_error{
	"Could not convert " + type_name<T> + " to string: buffer too small."};


  if (value >= 0)
    return std::string_view{nonneg_to_buf(begin, end - 1, value), end};

  constexpr T bottom{std::numeric_limits<T>::min()};
  if (value != bottom)
  {
    char *pos = nonneg_to_buf(begin, end, -value);
    *--pos = '\0';
    return std::string_view{pos, end - 1};
  }
  else
  {
    // This is the difficult case.  We can't negate the value, because on
    // two's-complement systems (basically every system nowadays), we'll get
    // the same value back.  We can't even check for that: the compiler is
    // allowed to pretend that such a thing won't happen.
    //
    // Luckily, there's only a limited number of such numbers.  We can cheat
    // and hard-code them all.
    return std::string_view{hard_neg<bottom + 1>};
  }
}


/// Helper: string traits implementation for built-in types.
/** These types all look much alike, so they can share much of their traits
 * classes (though templatised, of course).
 *
 * The actual `to_string` and `from_string` are implemented in the library,
 * but the rest is defined inline.
 */
template<typename TYPE> struct PQXX_LIBEXPORT builtin_traits
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(TYPE) { return false; }
  static void from_string(std::string_view str, TYPE &obj);
  static std::string to_string(TYPE obj);
};
} // namespace pqxx::internal


namespace pqxx
{
#if __has_include(<charconv>)
template<typename T> inline std::string_view
to_buf(char *begin, char *end, T value)
{
  const auto res = std::to_chars(begin, end - 1, value);
  if (res.ec != std::errc())
    throw conversion_error{
	pqxx::internal::make_conversion_error(res, type_name<T>)};
  *res.ptr = '\0';
  return std::string_view{begin, std::size_t(res.ptr - begin)};
}
#endif


template<> inline std::string_view
to_buf(char *, char *, bool value)
{
  // Define as char arrays first, to ensure trailing zero.
  static const char true_ptr[]{"true"}, false_ptr[]{"false"};
  static const std::string_view s_true{true_ptr}, s_false{false_ptr};
  return value ? s_true : s_false;
}


template<> inline std::string_view
to_buf(char *begin, char *end, const std::string &value)
{
  if (value.size() >= std::size_t(end - begin))
    throw conversion_error{
	"Could not convert string to string: too long for buffer."};
  std::memcpy(begin, value.c_str(), value.size() + 1);
  return std::string_view{begin, value.size()};
}


/// Default implementation of str uses @c to_buf.
template<typename T> class str
{
public:
  explicit str(T value) :
    m_view{to_buf(std::begin(m_buf), std::end(m_buf), value)}
  {}

  constexpr operator std::string_view() const noexcept { return m_view; }
  constexpr std::string_view view() const noexcept { return m_view; }
  const char *c_str() const noexcept { return m_buf; }

private:
  char m_buf[pqxx::internal::size_buffer<T>()];
  std::string_view m_view;
};


template<> class str<bool>
{
public:
  explicit str(bool value) : m_view{to_buf(nullptr, nullptr, value)} {}

  constexpr operator std::string_view() const noexcept { return m_view; }
  constexpr std::string_view view() const noexcept { return m_view; }
  const char *c_str() const noexcept { return m_view.data(); }

private:
  std::string_view m_view;
};


template<> class str<std::string>
{
public:
  explicit str(const std::string &value) : m_str{value} {}
  explicit str(std::string &&value) : m_str{std::move(value)} {}

  operator std::string_view() const noexcept { return m_str; }
  std::string_view view() const noexcept { return m_str; }
  const char *c_str() const noexcept { return m_str.c_str(); }

private:
  std::string m_str;
};


/// String traits for C-style string ("pointer to const char").
template<> struct PQXX_LIBEXPORT string_traits<const char *>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(const char *t) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }
  static void from_string(std::string_view str, const char *&obj)
	{ obj = str.data(); }
  static std::string to_string(const char *obj) { return obj; }
};

/// String traits for non-const C-style string ("pointer to char").
template<> struct PQXX_LIBEXPORT string_traits<char *>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(const char *t) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }

  // Don't allow this conversion since it breaks const-safety.
  // static void from_string(std::string_view str, char *&obj);

  static std::string to_string(char *obj) { return obj; }
};

/// String traits for C-style string constant ("array of char").
template<size_t N> struct PQXX_LIBEXPORT string_traits<char[N]>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(const char t[]) { return t == nullptr; }
  static constexpr const char *null() { return nullptr; }
  static std::string to_string(const char obj[]) { return obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<std::string>
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(const std::string &) { return false; }
  [[noreturn]] static std::string null()
	{ internal::throw_null_conversion(type_name<std::string>); }
  static void from_string(std::string_view str, std::string &obj)
	{ obj = str; }
  static std::string to_string(const std::string &obj) { return obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<const std::string>
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(const std::string &) { return false; }
  [[noreturn]] static const std::string null()
	{ internal::throw_null_conversion(type_name<std::string>); }
  static std::string to_string(const std::string &obj) { return obj; }
};

template<> struct PQXX_LIBEXPORT string_traits<std::stringstream>
{
  static constexpr bool has_null() noexcept { return false; }
  static constexpr bool is_null(const std::stringstream &) { return false; }
  [[noreturn]] static std::stringstream null()
	{ internal::throw_null_conversion(type_name<std::stringstream>); }
  static void from_string(std::string_view str, std::stringstream &obj)
	{ obj.clear(); obj << str; }
  static std::string to_string(const std::stringstream &obj)
	{ return obj.str(); }
};

/// Weird case: nullptr_t.  We don't fully support it.
template<> struct PQXX_LIBEXPORT string_traits<std::nullptr_t>
{
  static constexpr bool has_null() noexcept { return true; }
  static constexpr bool is_null(std::nullptr_t) noexcept { return true; }
  static constexpr std::nullptr_t null() { return nullptr; }
  static std::string to_string(const std::nullptr_t &)
	{ return "null"; }
};


// TODO: Implement date conversions.

} // namespace pqxx
