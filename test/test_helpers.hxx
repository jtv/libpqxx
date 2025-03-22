#include <stdexcept>
#include <string>

#include <pqxx/result>
#include <pqxx/row>

namespace pqxx
{
namespace test
{
class test_failure : public std::logic_error
{
public:
  test_failure(std::string const &desc, sl loc = sl::current());

  ~test_failure() noexcept override;

  constexpr char const *file() const noexcept { return m_loc.file_name(); }
  constexpr auto line() const noexcept { return m_loc.line(); }

private:
  sl m_loc;
};


/// Drop a table, if it exists.
void drop_table(
  transaction_base &, std::string const &table, sl loc = sl::current());


using testfunc = void (*)();


void register_test(char const name[], testfunc func);


/// Register a test while not inside a function.
struct registrar
{
  registrar(char const name[], testfunc func)
  {
    pqxx::test::register_test(name, func);
  }
};


// Register a test function, so the runner will run it.
#define PQXX_REGISTER_TEST(func)                                              \
  pqxx::test::registrar tst_##func                                            \
  {                                                                           \
    #func, func                                                               \
  }


// Unconditional test failure.
[[noreturn]] void check_notreached(
  std::string desc = "Execution was never supposed to reach this point.",
  sl loc = sl::current());

// Verify that a condition is met, similar to assert()
#define PQXX_CHECK(condition, desc)                                           \
  pqxx::test::check((condition), #condition, (desc))
void check(
  bool condition, char const text[], std::string const &desc,
  sl loc = sl::current());

// Verify that variable has the expected value.
#define PQXX_CHECK_EQUAL(actual, expected, desc)                              \
  pqxx::test::check_equal((actual), #actual, (expected), #expected, (desc))
template<typename ACTUAL, typename EXPECTED>
inline void check_equal(
  ACTUAL const &actual, char const actual_text[], EXPECTED const &expected,
  char const expected_text[], std::string const &desc, sl loc = sl::current())
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
  throw test_failure{fulldesc, loc};
}

// Verify that two values are not equal.
#define PQXX_CHECK_NOT_EQUAL(value1, value2, desc)                            \
  pqxx::test::check_not_equal((value1), #value1, (value2), #value2, (desc))
template<typename VALUE1, typename VALUE2>
inline void check_not_equal(
  VALUE1 const &value1, char const text1[], VALUE2 const &value2, char const text2[],
  std::string const &desc, sl loc = sl::current())
{
  if (value1 != value2)
    return;
  std::string const fulldesc = desc + " (" + text1 + " == " + text2 +
                               ": "
                               "both are " +
                               to_string(value2) + ")";
  throw test_failure{fulldesc, loc};
}


// Verify that value1 is less than value2.
#define PQXX_CHECK_LESS(value1, value2, desc)                                 \
  pqxx::test::check_less((value1), #value1, (value2), #value2, (desc))
// Verify that value1 is greater than value2.
#define PQXX_CHECK_GREATER(value2, value1, desc)                              \
  pqxx::test::check_less((value1), #value1, (value2), #value2, (desc))
template<typename VALUE1, typename VALUE2>
inline void check_less(
  VALUE1 const &value1, char const text1[], VALUE2 const &value2, char const text2[],
  std::string const &desc, sl loc = sl::current())
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


// Verify that value1 is less than or equal to value2.
#define PQXX_CHECK_LESS_EQUAL(value1, value2, desc)                           \
  pqxx::test::check_less_equal((value1), #value1, (value2), #value2, (desc))
// Verify that value1 is greater than or equal to value2.
#define PQXX_CHECK_GREATER_EQUAL(value2, value1, desc)                        \
  pqxx::test::check_less_equal((value1), #value1, (value2), #value2, (desc))
template<typename VALUE1, typename VALUE2>
inline void check_less_equal(
  VALUE1 const &value1, char const text1[], VALUE2 const &value2, char const text2[],
  std::string const &desc, sl loc = sl::current())
{
  if (value1 <= value2)
    return;
  std::string const fulldesc = pqxx::internal::concat(
    desc, " (", text1, " > ", text2, ": \"lower\"=", value1,
    ", \"upper\"=", value2, ")");
  throw test_failure{fulldesc, loc};
}


/// A special exception type not derived from `std::exception`.
struct failure_to_fail
{};


namespace internal
{
/// Syntactic placeholder: require (and accept) semicolon after block.
inline void end_of_statement() {}
} // namespace internal


// Verify that "action" does not throw an exception.
#define PQXX_CHECK_SUCCEEDS(action, desc)                                     \
  pqxx::test::check_succeeds(([&]() { action; }), #action, (desc))

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
      pqxx::internal::concat(
        desc, " - \"", text, "\" threw exception: ", e.what()),
      loc);
  }
  catch (...)
  {
    pqxx::test::check_notreached(
      pqxx::internal::concat(desc, " - \"", text, "\" threw a non-exception!"),
      loc);
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
      pqxx::internal::concat(desc, " (\"", text, "\" did not throw)"), loc);
  }
  catch (EXC const &)
  {}
  catch (std::exception const &e)
  {
    check_notreached(pqxx::internal::concat(
      desc, " (\"", text, "\" threw the wrong exception type: ", e.what(),
      ")"));
  }
  catch (...)
  {
    check_notreached(
      pqxx::internal::concat(
        desc, " (\"", text, "\" threw a non-exception type!)"),
      loc);
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
      pqxx::internal::concat(desc, " (\"", text, "\" did not throw)"), loc);
  }
  catch (std::exception const &)
  {}
  catch (...)
  {
    check_notreached(
      pqxx::internal::concat(
        desc, " (\"", text, "\" threw a non-exception type!)"),
      loc);
  }
}


// Verify that "action" throws an exception, of any std::exception-based type.
#define PQXX_CHECK_THROWS_EXCEPTION(action, desc)                             \
  pqxx::test::check_throws_exception(([&]() { action; }), #action, desc)

// Verify that "action" throws "exception_type" (which is not std::exception).
#define PQXX_CHECK_THROWS(action, exception_type, desc)                       \
  pqxx::test::check_throws<exception_type>(([&] { action; }), #action, desc)


#define PQXX_CHECK_BOUNDS(value, lower, upper, desc)                          \
  pqxx::test::check_bounds(                                                   \
    (value), #value, (lower), #lower, (upper), #upper, (desc))
template<typename VALUE, typename LOWER, typename UPPER>
inline void check_bounds(
  VALUE const &value, char const text[], LOWER const &lower, char const lower_text[],
  UPPER const &upper, char const upper_text[], std::string const &desc,
  sl loc = sl::current())
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
    desc + " (" + text + " is below lower bound " + lower_text + ")", loc);
  pqxx::test::check(
    value < upper, upper_check.c_str(),
    desc + " (" + text + " is not below upper bound " + upper_text + ")", loc);
}


// Report expected exception
void expected_exception(std::string const &);


// Represent result row as string.
std::string list_row(row);
// Represent result as string.
std::string list_result(result);
// Represent result iterator as string.
std::string list_result_iterator(result::const_iterator const &);


// @deprecated Set up test data for legacy tests.
void create_pqxxevents(transaction_base &);
} // namespace test


template<> inline std::string to_string(row const &value)
{
  return pqxx::test::list_row(value);
}


template<> inline std::string to_string(result const &value)
{
  return pqxx::test::list_result(value);
}


template<> inline std::string to_string(result::const_iterator const &value)
{
  return pqxx::test::list_result_iterator(value);
}
} // namespace pqxx
