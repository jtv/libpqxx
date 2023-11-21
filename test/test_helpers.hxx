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
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  test_failure(
    std::string const &desc,
    std::source_location loc = std::source_location::current());
#else
  test_failure(std::string const &ffile, int fline, std::string const &desc);
#endif

  ~test_failure() noexcept override;

#if defined(PQXX_HAVE_SOURCE_LOCATION)
  constexpr char const *file() const noexcept { return m_loc.file_name(); }
  constexpr auto line() const noexcept { return m_loc.line(); }
#else
  std::string const &file() const noexcept { return m_file; }
  int line() const noexcept { return m_line; }
#endif

private:
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  std::source_location m_loc;
#else
  std::string const m_file;
  int m_line;
#endif
};


/// Drop a table, if it exists.
void drop_table(transaction_base &, std::string const &table);


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
#if defined(PQXX_HAVE_SOURCE_LOCATION)
#  define PQXX_CHECK_NOTREACHED(desc) pqxx::test::check_notreached((desc))
#else
#  define PQXX_CHECK_NOTREACHED(desc)                                         \
    pqxx::test::check_notreached(__FILE__, __LINE__, (desc))
#endif
[[noreturn]] void check_notreached(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  std::string desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
);

// Verify that a condition is met, similar to assert()
#if defined(PQXX_HAVE_SOURCE_LOCATION)
#  define PQXX_CHECK(condition, desc)                                         \
    pqxx::test::check((condition), #condition, (desc))
#else
#  define PQXX_CHECK(condition, desc)                                         \
    pqxx::test::check(__FILE__, __LINE__, (condition), #condition, (desc))
#endif
void check(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  bool condition, char const text[], std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
);

// Verify that variable has the expected value.
#if defined(PQXX_HAVE_SOURCE_LOCATION)
#  define PQXX_CHECK_EQUAL(actual, expected, desc)                            \
    pqxx::test::check_equal((actual), #actual, (expected), #expected, (desc))
#else
#  define PQXX_CHECK_EQUAL(actual, expected, desc)                            \
    pqxx::test::check_equal(                                                  \
      __FILE__, __LINE__, (actual), #actual, (expected), #expected, (desc))
#endif
template<typename ACTUAL, typename EXPECTED>
inline void check_equal(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  ACTUAL actual, char const actual_text[], EXPECTED expected,
  char const expected_text[], std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
)
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
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  throw test_failure{fulldesc, loc};
#else
  throw test_failure(file, line, fulldesc);
#endif
}

// Verify that two values are not equal.
#if defined(PQXX_HAVE_SOURCE_LOCATION)
#  define PQXX_CHECK_NOT_EQUAL(value1, value2, desc)                          \
    pqxx::test::check_not_equal((value1), #value1, (value2), #value2, (desc))
#else
#  define PQXX_CHECK_NOT_EQUAL(value1, value2, desc)                          \
    pqxx::test::check_not_equal(                                              \
      __FILE__, __LINE__, (value1), #value1, (value2), #value2, (desc))
#endif
template<typename VALUE1, typename VALUE2>
inline void check_not_equal(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  VALUE1 value1, char const text1[], VALUE2 value2, char const text2[],
  std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
)
{
  if (value1 != value2)
    return;
  std::string const fulldesc = desc + " (" + text1 + " == " + text2 +
                               ": "
                               "both are " +
                               to_string(value2) + ")";
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  throw test_failure{fulldesc, loc};
#else
  throw test_failure{file, line, fulldesc};
#endif
}


#if defined(PQXX_HAVE_SOURCE_LOCATION)
// Verify that value1 is less than value2.
#  define PQXX_CHECK_LESS(value1, value2, desc)                               \
    pqxx::test::check_less((value1), #value1, (value2), #value2, (desc))
// Verify that value1 is greater than value2.
#  define PQXX_CHECK_GREATER(value2, value1, desc)                            \
    pqxx::test::check_less((value1), #value1, (value2), #value2, (desc))
#else
// Verify that value1 is less than value2.
#  define PQXX_CHECK_LESS(value1, value2, desc)                               \
    pqxx::test::check_less(                                                   \
      __FILE__, __LINE__, (value1), #value1, (value2), #value2, (desc))
// Verify that value1 is greater than value2.
#  define PQXX_CHECK_GREATER(value2, value1, desc)                            \
    pqxx::test::check_less(                                                   \
      __FILE__, __LINE__, (value1), #value1, (value2), #value2, (desc))
#endif
template<typename VALUE1, typename VALUE2>
inline void check_less(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  VALUE1 value1, char const text1[], VALUE2 value2, char const text2[],
  std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
)
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
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  throw test_failure{fulldesc, loc};
#else
  throw test_failure(file, line, fulldesc);
#endif
}


#if defined(PQXX_HAVE_SOURCE_LOCATION)
// Verify that value1 is less than or equal to value2.
#  define PQXX_CHECK_LESS_EQUAL(value1, value2, desc)                         \
    pqxx::test::check_less_equal((value1), #value1, (value2), #value2, (desc))
// Verify that value1 is greater than or equal to value2.
#  define PQXX_CHECK_GREATER_EQUAL(value2, value1, desc)                      \
    pqxx::test::check_less_equal((value1), #value1, (value2), #value2, (desc))
#else
// Verify that value1 is less than or equal to value2.
#  define PQXX_CHECK_LESS_EQUAL(value1, value2, desc)                         \
    pqxx::test::check_less_equal(                                             \
      __FILE__, __LINE__, (value1), #value1, (value2), #value2, (desc))
// Verify that value1 is greater than or equal to value2.
#  define PQXX_CHECK_GREATER_EQUAL(value2, value1, desc)                      \
    pqxx::test::check_less_equal(                                             \
      __FILE__, __LINE__, (value1), #value1, (value2), #value2, (desc))
#endif
template<typename VALUE1, typename VALUE2>
inline void check_less_equal(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  VALUE1 value1, char const text1[], VALUE2 value2, char const text2[],
  std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
)
{
  if (value1 <= value2)
    return;
  std::string const fulldesc = desc + " (" + text1 + " > " + text2 +
                               ": "
                               "\"lower\"=" +
                               to_string(value1) +
                               ", "
                               "\"upper\"=" +
                               to_string(value2) + ")";
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  throw test_failure{fulldesc, loc};
#else
  throw test_failure(file, line, fulldesc);
#endif
}


struct failure_to_fail
{};


namespace internal
{
/// Syntactic placeholder: require (and accept) semicolon after block.
inline void end_of_statement() {}
} // namespace internal


// Verify that "action" does not throw an exception.
#define PQXX_CHECK_SUCCEEDS(action, desc)                                     \
  {                                                                           \
    try                                                                       \
    {                                                                         \
      action;                                                                 \
    }                                                                         \
    catch (std::exception const &e)                                           \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} + " - \"" +                                         \
        #action "\" threw exception: " + e.what());                           \
    }                                                                         \
    catch (...)                                                               \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} + " - \"" + #action "\" threw a non-exception!");   \
    }                                                                         \
  }                                                                           \
  pqxx::test::internal::end_of_statement()

// Verify that "action" throws an exception, of any std::exception-based type.
#define PQXX_CHECK_THROWS_EXCEPTION(action, desc)                             \
  {                                                                           \
    try                                                                       \
    {                                                                         \
      action;                                                                 \
      throw pqxx::test::failure_to_fail();                                    \
    }                                                                         \
    catch (pqxx::test::failure_to_fail const &)                               \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} + " (\"" #action "\" did not throw)");              \
    }                                                                         \
    catch (std::exception const &)                                            \
    {}                                                                        \
    catch (...)                                                               \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} + " (\"" #action "\" threw non-exception type)");   \
    }                                                                         \
  }                                                                           \
  pqxx::test::internal::end_of_statement()

// Verify that "action" throws "exception_type" (which is not std::exception).
#define PQXX_CHECK_THROWS(action, exception_type, desc)                       \
  {                                                                           \
    try                                                                       \
    {                                                                         \
      action;                                                                 \
      throw pqxx::test::failure_to_fail();                                    \
    }                                                                         \
    catch (pqxx::test::failure_to_fail const &)                               \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} + " (\"" #action                                    \
                            "\" did not throw " #exception_type ")");         \
    }                                                                         \
    catch (exception_type const &)                                            \
    {}                                                                        \
    catch (std::exception const &e)                                           \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} +                                                   \
        " (\"" #action                                                        \
        "\" "                                                                 \
        "threw exception other than " #exception_type ": " +                  \
        e.what() + ")");                                                      \
    }                                                                         \
    catch (...)                                                               \
    {                                                                         \
      PQXX_CHECK_NOTREACHED(                                                  \
        std::string{desc} + " (\"" #action "\" threw non-exception type)");   \
    }                                                                         \
  }                                                                           \
  pqxx::test::internal::end_of_statement()

#if defined(PQXX_HAVE_SOURCE_LOCATION)
#  define PQXX_CHECK_BOUNDS(value, lower, upper, desc)                        \
    pqxx::test::check_bounds(                                                 \
      (value), #value, (lower), #lower, (upper), #upper, (desc))
#else
#  define PQXX_CHECK_BOUNDS(value, lower, upper, desc)                        \
    pqxx::test::check_bounds(                                                 \
      __FILE__, __LINE__, (value), #value, (lower), #lower, (upper), #upper,  \
      (desc))
#endif
template<typename VALUE, typename LOWER, typename UPPER>
inline void check_bounds(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  VALUE value, char const text[], LOWER lower, char const lower_text[],
  UPPER upper, char const upper_text[], std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc = std::source_location::current()
#endif
)
{
  std::string const range_check = std::string{lower_text} + " < " + upper_text,
                    lower_check =
                      std::string{"!("} + text + " < " + lower_text + ")",
                    upper_check = std::string{text} + " < " + upper_text;

  pqxx::test::check(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
    file, line,
#endif
    lower < upper, range_check.c_str(),
    desc + " (acceptable range is empty; value was " + text + ")"
#if defined(PQXX_HAVE_SOURCE_LOCATION)
    ,
    loc
#endif
  );
  pqxx::test::check(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
    file, line,
#endif
    not(value < lower), lower_check.c_str(),
    desc + " (" + text + " is below lower bound " + lower_text + ")"
#if defined(PQXX_HAVE_SOURCE_LOCATION)
    ,
    loc
#endif
  );
  pqxx::test::check(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
    file, line,
#endif
    value < upper, upper_check.c_str(),
    desc + " (" + text + " is not below upper bound " + upper_text + ")"
#if defined(PQXX_HAVE_SOURCE_LOCATION)
    ,
    loc
#endif
  );
}


// Report expected exception
void expected_exception(std::string const &);


// Represent result row as string.
std::string list_row(row);
// Represent result as string.
std::string list_result(result);
// Represent result iterator as string.
std::string list_result_iterator(result::const_iterator);


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
