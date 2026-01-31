#if !defined(PQXX_H_TEST_HELPERS)
#  define PQXX_H_TEST_HELPERS

#  include <mutex>
#  include <random>
#  include <stdexcept>
#  include <string>

#  include <pqxx/result>
#  include <pqxx/row>

namespace pqxx::test
{
/// Random engine, for generating random values in tests.
using randomizer = std::mt19937;


/// Context for running a test.
/** Defines various utilities that can help tests: randomisers, a temporary
 * filesystem directory.
 */
struct context
{
  /// Create a context for one thread to run tests.
  /** Seeds the randommiser with a highly predictable 0 initially.  Call the
   * @ref seed() function before consuming random values.
   */
  context(std::size_t random_seed) : rnd{0}, rnd_seed{random_seed} {}

  context() = delete;
  context(context const &) = delete;
  context(context &&) = delete;
  context &operator=(context const &) = delete;
  context &operator=(context &&) = delete;

  /// Seed the randomiser using the original seed.
  /** Do this before every individual test to get reproducible test sequences.
   */
  void seed(std::string_view test_name)
  {
    auto const seed{static_cast<randomizer::result_type>(
      rnd_seed ^ string_hasher(test_name))};
    rnd.seed(seed);
  }

  /// Return an arbitrary nonnegative integer.
  int make_num() { return static_cast<int>(rnd() >> 1); }

  /// Return an arbitrary nonnegative integer below `ceiling`.
  int make_num(int ceiling) { return make_num() % ceiling; }

  /// Return an arbitrary nonzero `char` value from the full 8-bit range.
  char random_char()
  {
    return static_cast<char>(static_cast<std::uint8_t>(make_num(255) + 1));
  }

  /// Return an arbitrary numeric floating-point value.  (No NaN or infinity.)
  template<std::floating_point T> T make_float_num()
  {
    auto const x{make_num()}, z{make_num()};
    auto y{make_num()};
    while (y == x) y = make_num();
    return static_cast<T>(x - y) / static_cast<T>(z);
  }

  /// Generate a name with a given prefix and a randomised suffix.
  std::string make_name(std::string_view prefix = {})
  {
    int const num{make_num()};
    // TODO: Use wider character set than just those digits.
    if (std::empty(prefix))
      return std::format("{}", num);
    else
      return std::format("{}_{}", prefix, num);
  }

private:
  /// A random engine.
  randomizer rnd;

  /// The random seed
  std::size_t rnd_seed;

  /// A factory of hash values for strings.
  std::hash<std::string_view> string_hasher;
};


/// Exception: A test does not satisfy expected condition.
class test_failure : public std::logic_error
{
public:
  test_failure(std::string const &desc, sl loc = sl::current());
  ~test_failure() noexcept override;

  sl const &location() const noexcept { return m_loc; }

  std::string_view name() const noexcept { return "Failure"; }

private:
  test_failure &operator=(test_failure const &) = delete;

  sl const m_loc;
};


/// For use by tests that need to simulate an exception.
struct deliberate_error : std::exception
{};


/// Drop a table, if it exists.
void drop_table(
  transaction_base &, std::string const &table, sl loc = sl::current());


using testfunc = void (*)(pqxx::test::context &);


/// Maximum number of tests in the test suite.
/** If this should prove insufficient, increase it.
 */
constexpr inline std::size_t max_tests{1000};


/// The test suite.
/** This is where the tests get registered at initialisation time.
 *
 * This gets a bit hacky.  It relies on an internal counter being
 * zero-initialised before the test registrations get constructed.
 */
class suite
{
public:
  /// Register a test function.
  static void register_test(std::string_view name, testfunc func) noexcept;

  /// Collect all tests into a map: test name to test function.
  static std::map<std::string_view, testfunc> gather();

private:
  /// Number of registered tests.
  static constinit std::size_t s_num_tests;

  static constinit std::array<std::string_view, max_tests> s_names;
  static constinit std::array<testfunc, max_tests> s_funcs;
};


// Register a test function, so the runner will run it.
#  define PQXX_REGISTER_TEST(func)                                            \
    [[maybe_unused]] pqxx::test::registrar const tst_##func                   \
    {                                                                         \
      #func, func                                                             \
    }


/// Register a test while not inside a function.
struct registrar final
{
  registrar(char const name[], testfunc func) noexcept
  {
    pqxx::test::suite::register_test(name, func);
  }
};


// Unconditional test failure.
[[noreturn]] void check_notreached(
  std::string const &desc =
    "Execution was never supposed to reach this point.",
  sl loc = sl::current());

// Verify that a condition is met, similar to assert().
// Takes an optional failure description as a second argument.
#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.
#    define PQXX_CHECK(condition, ...)                                        \
      pqxx::test::check((condition), #condition, ##__VA_ARGS__)
#  else
#    define PQXX_CHECK(condition, ...)                                        \
      pqxx::test::check((condition), #condition __VA_OPT__(, ) __VA_ARGS__)
#  endif

void check(
  bool condition, char const text[],
  std::string const &desc = "Condition check failed,", sl loc = sl::current());

// Verify that variable has the expected value.
// Takes an optional failure description as a third argument.
#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.
#    define PQXX_CHECK_EQUAL(actual, expected, ...)                           \
      pqxx::test::check_equal(                                                \
        (actual), #actual, (expected), #expected, ##__VA_ARGS__)
#  else
#    define PQXX_CHECK_EQUAL(actual, expected, ...)                           \
      pqxx::test::check_equal(                                                \
        (actual), #actual, (expected), #expected __VA_OPT__(, ) __VA_ARGS__)
#  endif

template<typename ACTUAL, typename EXPECTED>
inline void check_equal(
  ACTUAL const &actual, char const actual_text[], EXPECTED const &expected,
  char const expected_text[],
  std::string const &desc = "Equality check failed.", sl loc = sl::current())
{
  if (expected == actual)
    return;
  std::string const fulldesc = desc + " (" + actual_text + " <> " +
                               expected_text +
                               ": "
                               "actual=" +
                               to_string(actual) +
                               ", "
                               "expected=" +
                               to_string(expected) + ")";
  throw test_failure{
    std::format(
      "{}\n{} <> {}.\nExpected: {}\nActual:   {}", desc, actual_text,
      expected_text, to_string(expected), to_string(actual)),
    loc};
}

// Verify that two values are not equal.
// Takes an optional failure description as a third argument.
#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.
#    define PQXX_CHECK_NOT_EQUAL(value1, value2, ...)                         \
      pqxx::test::check_not_equal(                                            \
        (value1), #value1, (value2), #value2, ##__VA_ARGS__)
#  else
#    define PQXX_CHECK_NOT_EQUAL(value1, value2, ...)                         \
      pqxx::test::check_not_equal(                                            \
        (value1), #value1, (value2), #value2 __VA_OPT__(, ) __VA_ARGS__)
#  endif
template<typename VALUE1, typename VALUE2>
inline void check_not_equal(
  VALUE1 const &value1, char const text1[], VALUE2 const &value2,
  char const text2[], std::string const &desc = "Inequality check failed.",
  sl loc = sl::current())
{
  if (value1 != value2)
    return;
  std::string const fulldesc = desc + " (" + text1 + " == " + text2 +
                               ": "
                               "both are " +
                               to_string(value2) + ")";
  throw test_failure{fulldesc, loc};
}


// Verify that value1 is less/greater than value2.
// Takes an optional failure description as a third argument.
#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.
#    define PQXX_CHECK_LESS(value1, value2, ...)                              \
      pqxx::test::check_less(                                                 \
        (value1), #value1, (value2), #value2, ##__VA_ARGS__)
#    define PQXX_CHECK_GREATER(value2, value1, ...)                           \
      pqxx::test::check_less(                                                 \
        (value1), #value1, (value2), #value2, ##__VA_ARGS__)
#  else
#    define PQXX_CHECK_LESS(value1, value2, ...)                              \
      pqxx::test::check_less(                                                 \
        (value1), #value1, (value2), #value2 __VA_OPT__(, ) __VA_ARGS__)
#    define PQXX_CHECK_GREATER(value2, value1, ...)                           \
      pqxx::test::check_less(                                                 \
        (value1), #value1, (value2), #value2 __VA_OPT__(, ) __VA_ARGS__)
#  endif
template<typename VALUE1, typename VALUE2>
inline void check_less(
  VALUE1 const &value1, char const text1[], VALUE2 const &value2,
  char const text2[], std::string const &desc = "Less/greater check failed.",
  sl loc = sl::current())
{
  if (value1 < value2)
    return;
  std::string const fulldesc = desc + " (" + text1 + " >= " + text2 +
                               ": "
                               "\"lower\"=" +
                               to_string(value1) +
                               ", "
                               "\"upper\"=" +
                               to_string(value2) + ")";
  throw test_failure{fulldesc, loc};
}


// Verify that value1 is less/greater than or equal to value2.
// Takes an optional failure description as a third argument.
#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.
#    define PQXX_CHECK_LESS_EQUAL(value1, value2, ...)                        \
      pqxx::test::check_less_equal(                                           \
        (value1), #value1, (value2), #value2, ##__VA_ARGS__)
#    define PQXX_CHECK_GREATER_EQUAL(value2, value1, ...)                     \
      pqxx::test::check_less_equal(                                           \
        (value1), #value1, (value2), #value2, ##__VA_ARGS__)
#  else
#    define PQXX_CHECK_LESS_EQUAL(value1, value2, ...)                        \
      pqxx::test::check_less_equal(                                           \
        (value1), #value1, (value2), #value2 __VA_OPT__(, ) __VA_ARGS__)
#    define PQXX_CHECK_GREATER_EQUAL(value2, value1, ...)                     \
      pqxx::test::check_less_equal(                                           \
        (value1), #value1, (value2), #value2 __VA_OPT__(, ) __VA_ARGS__)
#  endif
template<typename VALUE1, typename VALUE2>
inline void check_less_equal(
  VALUE1 const &value1, char const text1[], VALUE2 const &value2,
  char const text2[], std::string const &desc = "Less/greater check failed.",
  sl loc = sl::current())
{
  if (value1 <= value2)
    return;
  std::string fulldesc{std::format(
    "{} ({} > {}: \"lower\"={}, \"upper\"={})", desc, text1, text2, value1,
    value2)};
  throw test_failure{fulldesc, loc};
}


/// A special exception type not derived from `std::exception`.
struct failure_to_fail final
{};


namespace internal
{
/// Syntactic placeholder: require (and accept) semicolon after block.
inline void end_of_statement() {}
} // namespace internal


// Verify that "action" does not throw an exception.
// Takes an optional failure description as a second argument.
#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.
#    define PQXX_CHECK_SUCCEEDS(action, ...)                                  \
      pqxx::test::check_succeeds(([&]() { action; }), #action, ##__VA_ARGS__)
#  else
#    define PQXX_CHECK_SUCCEEDS(action, ...)                                  \
      pqxx::test::check_succeeds(                                             \
        ([&]() { action; }), #action __VA_OPT__(, ) __VA_ARGS__)
#  endif

template<std::invocable F>
inline void check_succeeds(
  F &&f, char const text[], std::string desc = "Expected this to succeed.",
  sl loc = sl::current())
{
  try
  {
    f();
  }
  catch (std::exception const &e)
  {
    pqxx::test::check_notreached(
      std::format("{} - \"{}\" threw exception: {}", desc, text, e.what()),
      loc);
  }
  catch (...)
  {
    pqxx::test::check_notreached(
      std::format("{} - \"{}\" threw a non-exception!", desc, text), loc);
  }
}


template<typename EXC, std::invocable F>
inline void check_throws(
  F &&f, char const text[],
  std::string desc = "This code did not thow the expected exception.",
  pqxx::sl loc = sl::current())
{
  try
  {
    f();
    throw failure_to_fail{};
  }
  catch (failure_to_fail const &)
  {
    check_notreached(
      std::format("{} (\"{}\" did not throw).", desc, text), loc);
  }
  catch (EXC const &)
  {
    // OK, that's the exception we wanted.
    return;
  }
  catch (std::exception const &e)
  {
    auto const fail{dynamic_cast<pqxx::failure const *>(&e)};
    if (fail == nullptr)
    {
      check_notreached(
        std::format(
          "{} (\"{}\" threw the wrong exception type: {}).", desc, text,
          e.what()),
        loc);
    }
    else
    {
      check_notreached(
        std::format(
          "{} (\"{}\" threw the wrong exception type: [{}] {}).", desc, text,
          fail->name(), e.what()),
        loc);
    }
  }
  catch (...)
  {
    check_notreached(
      std::format("{} (\"{}\" threw a non-exception type!)", desc, text), loc);
  }
}


template<std::invocable F>
inline void check_throws_exception(
  F &&f, char const text[],
  std::string desc = "This code did not thow a std::exception.",
  pqxx::sl loc = sl::current())
{
  try
  {
    f();
    throw failure_to_fail{};
  }
  catch (failure_to_fail const &)
  {
    check_notreached(
      std::format("{} (\"{}\" did not throw)", desc, text), loc);
  }
  catch (std::exception const &)
  {}
  catch (...)
  {
    check_notreached(
      std::format("{} (\"{}\" threw a non-exception type).", desc, text), loc);
  }
}


#  if defined(_MSVC_TRADITIONAL) && _MSVC_TRADITIONAL
// Work around Visual Studio lacking support for __VA_OPT__.

// Verify that "action" throws an exception, of any std::exception-based type.
// Takes an optional failure description as an 2nd argument.
#    define PQXX_CHECK_THROWS_EXCEPTION(action, ...)                          \
      pqxx::test::check_throws_exception(                                     \
        ([&]() { return action, 0; }), #action, ##__VA_ARGS__)

// Verify that "action" throws "exception_type" (which is not std::exception).
// Takes an optional failure description as an 2nd argument.
#    define PQXX_CHECK_THROWS(action, exception_type, ...)                    \
      pqxx::test::check_throws<exception_type>(                               \
        ([&] { return action, 0; }), #action, ##__VA_ARGS__)


// Verify that "value" is between "lower" (inclusive) and "upper" (exclusive).
// Takes an optional failure description as a 4th argument.
#    define PQXX_CHECK_BOUNDS(value, lower, upper, ...)                       \
      pqxx::test::check_bounds(                                               \
        (value), #value, (lower), #lower, (upper), #upper, ##__VA_ARGS__)

#  else

// Verify that "action" throws an exception, of any std::exception-based type.
// Takes an optional failure description as an 2nd argument.
#    define PQXX_CHECK_THROWS_EXCEPTION(action, ...)                          \
      pqxx::test::check_throws_exception(                                     \
        ([&]() { return action, 0; }), #action __VA_OPT__(, ) __VA_ARGS__)

// Verify that "action" throws "exception_type" (which is not std::exception).
// Takes an optional failure description as an 2nd argument.
#    define PQXX_CHECK_THROWS(action, exception_type, ...)                    \
      pqxx::test::check_throws<exception_type>(                               \
        ([&] { return action, 0; }), #action __VA_OPT__(, ) __VA_ARGS__)


// Verify that "value" is between "lower" (inclusive) and "upper" (exclusive).
// Takes an optional failure description as a 4th argument.
#    define PQXX_CHECK_BOUNDS(value, lower, upper, ...)                       \
      pqxx::test::check_bounds(                                               \
        (value), #value, (lower), #lower, (upper),                            \
        #upper __VA_OPT__(, ) __VA_ARGS__)


#  endif
template<typename VALUE, typename LOWER, typename UPPER>
inline void check_bounds(
  VALUE const &value, char const text[], LOWER const &lower,
  char const lower_text[], UPPER const &upper, char const upper_text[],
  std::string const &desc = "Bounds check failed.", sl loc = sl::current())
{
  std::string const range_check = std::string{lower_text} + " < " + upper_text,
                    lower_check =
                      std::string{"!("} + text + " < " + lower_text + ")",
                    upper_check = std::string{text} + " < " + upper_text;

  pqxx::test::check(
    lower < upper, range_check.c_str(),
    desc + " (acceptable range is empty; value was " + text + ")", loc);
  pqxx::test::check(
    not(value < lower), lower_check.c_str(),
    std::format(
      "{} ({} is below lower bound {}: {} < {})", desc, text, lower_text,
      value, lower),
    loc);
  pqxx::test::check(
    value < upper, upper_check.c_str(),
    std::format(
      "{} ({} is not below upper bound {}: {} > {})", desc, text, upper_text,
      value, upper),
    loc);
}


// Report expected exception
void expected_exception(std::string const &);


// Represent result row as string.
std::string list_row(row);
// Represent result row as string.
std::string list_row(row_ref);
// Represent result as string.
std::string list_result(result);
// Represent result iterator as string.
std::string list_result_iterator(result::const_iterator const &);


// @deprecated Set up test data for legacy tests.
void create_pqxxevents(transaction_base &);
} // namespace pqxx::test


namespace pqxx
{
template<> inline std::string to_string(row const &value, ctx)
{
  return pqxx::test::list_row(value);
}


template<> inline std::string to_string(result const &value, ctx)
{
  return pqxx::test::list_result(value);
}


template<>
inline std::string to_string(result::const_iterator const &value, ctx)
{
  return pqxx::test::list_result_iterator(value);
}
} // namespace pqxx

#endif
