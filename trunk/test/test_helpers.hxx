#include <map>

#include <pqxx/pqxx>


namespace pqxx
{
namespace test
{
class test_failure : public std::logic_error
{
  const std::string m_file;
  int m_line;

public:
  test_failure(
	const std::string &ffile,
	int fline,
	const std::string &desc);

  ~test_failure() PQXX_NOEXCEPT;

  const std::string &file() const PQXX_NOEXCEPT { return m_file; }
  int line() const PQXX_NOEXCEPT { return m_line; }
};


/// For backends that don't have generate_series(): sequence of ints
/** If the backend lacks generate_series(), prepares a temp table called
 * series" containing given range of numbers (including lowest and highest).
 *
 * Use select_series() to construct a query selecting a range of numbers.  For
 * the workaround on older backends to work, the ranges of numbers passed to
 * select_series() must be subsets of the range passed here.
 */
void prepare_series(transaction_base &t, int lowest, int highest);


/// Generate query selecting series of numbers from lowest to highest, inclusive
/** Needs to see connection object to determine whether the backend supports
 * generate_series().
 */
std::string select_series(connection_base &conn, int lowest, int highest);


/// Drop a table, if it exists.
void drop_table(transaction_base &, const std::string &table);


class base_test;
typedef std::map<std::string, base_test *> test_map;

/// Register test (if given); return test_map.
const test_map &register_test(base_test *);


/// Base class for test cases.
class base_test
{
public:
  typedef void (*testfunc)(transaction_base &);

  base_test(const std::string &tname, testfunc func);

  /// Overridable: run test case.
  virtual void run() =0;

  virtual ~base_test() =0;
  const std::string &name() const PQXX_NOEXCEPT { return m_name; }
private:
  std::string m_name;
protected:
  testfunc m_func;
};


/// Runner class for libpqxx tests.  Sets up a connection and transaction.
template<typename CONNECTION=connection, typename TRANSACTION=work>
class test_case PQXX_FINAL : public base_test
{
public:
  // func takes connection and transaction as arguments.
  test_case(const std::string &tname, testfunc func) :
    base_test(tname, func)
  {
  }

  ~test_case() {}

  virtual void run() PQXX_OVERRIDE
  {
    CONNECTION c;
    TRANSACTION t(c, name());

    // Workaround for older backend versions that lack generate_series().
    prepare_series(t, 0, 100);

    m_func(t);
  }
};


// Register a function taking (connection_base &, transaction_base &) as a test.
#define PQXX_REGISTER_TEST(function) \
	namespace \
	{ \
	pqxx::test::test_case<> test(#function, function); \
	}

// Register a test function using given connection and transaction types.
#define PQXX_REGISTER_TEST_CT(function, connection_type, transaction_type) \
	namespace \
	{ \
	pqxx::test::test_case< connection_type, transaction_type > \
		test(#function, function); \
	}

// Register a test function using a given connection type (instead of the
// default "connection").
#define PQXX_REGISTER_TEST_C(function, connection_type) \
	PQXX_REGISTER_TEST_CT(function, connection_type, pqxx::work)

// Register a test function using a given transaction type (default is "work").
#define PQXX_REGISTER_TEST_T(function, transaction_type) \
	PQXX_REGISTER_TEST_CT(function, pqxx::connection, transaction_type)


// Register test function that takes a nullconnection and nontransaction.
#define PQXX_REGISTER_TEST_NODB(function) \
	PQXX_REGISTER_TEST_CT( \
		function, \
		pqxx::nullconnection, \
		pqxx::nontransaction)


// Unconditional test failure.
#define PQXX_CHECK_NOTREACHED(desc) \
	pqxx::test::check_notreached(__FILE__, __LINE__, (desc))
PQXX_NORETURN void check_notreached( \
	const char file[], \
	int line, \
	std::string desc);

// Verify that a condition is met, similar to assert()
#define PQXX_CHECK(condition, desc) \
	pqxx::test::check(__FILE__, __LINE__, (condition), #condition, (desc))
void check(
	const char file[],
	int line,
	bool condition,
	const char text[], 
	std::string desc);

// Verify that variable has the expected value.
#define PQXX_CHECK_EQUAL(actual, expected, desc) \
	pqxx::test::check_equal( \
		__FILE__, \
		__LINE__, \
		(actual), \
		#actual, \
		(expected), \
		#expected, \
		(desc))
template<typename ACTUAL, typename EXPECTED>
inline void check_equal(
	const char file[],
	int line,
	ACTUAL actual,
	const char actual_text[],
	EXPECTED expected, 
	const char expected_text[],
	std::string desc)
{
  if (expected == actual) return;
  const std::string fulldesc =
	desc + " (" + actual_text + " <> " + expected_text + ": "
	"actual=" + to_string(actual) + ", "
	"expected=" + to_string(expected) + ")";
  throw test_failure(file, line, fulldesc);
}

// Verify that two values are not equal.
#define PQXX_CHECK_NOT_EQUAL(value1, value2, desc) \
	pqxx::test::check_not_equal( \
		__FILE__, \
		__LINE__, \
		(value1), \
		#value1, \
		(value2), \
		#value2, \
		(desc))
template<typename VALUE1, typename VALUE2>
inline void check_not_equal(
	const char file[],
	int line,
	VALUE1 value1,
	const char text1[],
	VALUE2 value2, 
	const char text2[],
	std::string desc)
{
  if (value1 != value2) return;
  const std::string fulldesc =
	desc + " (" + text1 + " == " + text2 + ": "
	"both are " + to_string(value2) + ")";
  throw test_failure(file, line, fulldesc);
}


// Verify that value1 is less than value2.
#define PQXX_CHECK_LESS(value1, value2, desc) \
	pqxx::test::check_less( \
		__FILE__, \
		__LINE__, \
		(value1), \
		#value1, \
		(value2), \
		#value2, \
		(desc))
template<typename VALUE1, typename VALUE2>
inline void check_less(
	const char file[],
	int line,
	VALUE1 value1,
	const char text1[],
	VALUE2 value2,
	const char text2[],
	std::string desc)
{
  if (value1 < value2) return;
  const std::string fulldesc =
	desc + " (" + text1 + " >= " + text2 + ": "
	"\"lower\"=" + to_string(value1) + ", "
	"\"upper\"=" + to_string(value2) + ")";
  throw test_failure(file, line, fulldesc);
}


struct failure_to_fail {};


namespace internal
{
/// Syntactic placeholder: require (and accept) semicolon after block.
inline void end_of_statement()
{
}
}


// Verify that "action" throws "exception_type."
#define PQXX_CHECK_THROWS(action, exception_type, desc) \
	{ \
	  try \
	  { \
	    action ; \
	    throw pqxx::test::failure_to_fail(); \
	  } \
	  catch (const pqxx::test::failure_to_fail &) \
	  { \
	    PQXX_CHECK_NOTREACHED( \
		std::string(desc) + \
		" (\"" #action "\" did not throw " #exception_type ")"); \
	  } \
	  catch (const exception_type &) {} \
	  catch (...) \
	  { \
	    PQXX_CHECK_NOTREACHED( \
		std::string(desc) + \
		" (\"" #action "\" " \
		"threw exception other than " #exception_type ")"); \
	  } \
	} \
        pqxx::test::internal::end_of_statement()

#define PQXX_CHECK_BOUNDS(value, lower, upper, desc) \
	pqxx::test::check_bounds( \
		__FILE__, \
		 __LINE__, \
		(value), \
		#value, \
		(lower), \
		#lower, \
		(upper), \
		#upper, \
		(desc))
template<typename VALUE, typename LOWER, typename UPPER>
inline void check_bounds(
	const char file[],
	int line,
	VALUE value,
	const char text[],
	LOWER lower,
	const char lower_text[],
	UPPER upper,
	const char upper_text[],
	const std::string &desc)
{
  const std::string
	range_check = std::string(lower_text) + " < " + upper_text,
	lower_check = std::string("!(") + text + " < " + lower_text + ")",
	upper_check = std::string(text) + " < " + upper_text;

  pqxx::test::check(
	file,
	line,
	lower < upper,
	range_check.c_str(),
	desc + " (acceptable range is empty; value was " + text + ")");
  pqxx::test::check(
	file,
	line,
	!(value < lower),
	lower_check.c_str(),
	desc + " (" + text + " is below lower bound " + lower_text + ")");
  pqxx::test::check(
	file,
	line,
	value < upper,
	upper_check.c_str(),
	desc + " (" + text + " is not below upper bound " + upper_text + ")");
}


// Report expected exception
void expected_exception(const std::string &);


// Represent result row as string
std::string list_row(row);
// Represent result as string
std::string list_result(result);
// Represent result iterator as string
std::string list_result_iterator(result::const_iterator);


// @deprecated Set up test data for legacy tests.
void create_pqxxevents(transaction_base &);
} // namespace test


// Support string conversion on result rows for debug output.
template<> struct string_traits<row>
{
  static const char *name() { return "pqxx::row"; }
  static bool has_null() { return false; }
  static bool is_null(row) { return false; }
  static result null(); // Not needed
  static void from_string(const char Str[], result &Obj); // Not needed
  static std::string to_string(row Obj)
	{ return pqxx::test::list_row(Obj); }
};

// Support string conversion on result objects for debug output.
template<> struct string_traits<result>
{
  static const char *name() { return "pqxx::result"; }
  static bool has_null() { return true; }
  static bool is_null(result r) { return r.empty(); }
  static result null() { return result(); }
  static void from_string(const char Str[], result &Obj); // Not needed
  static std::string to_string(result Obj)
	{ return pqxx::test::list_result(Obj); }
};

// Support string conversion on result::const_iterator for debug output.
template<> struct string_traits<result::const_iterator>
{
  typedef result::const_iterator subject_type;
  static const char *name() { return "pqxx::result::const_iterator"; }
  static bool has_null() { return false; }
  static bool is_null(subject_type) { return false; }
  static result null(); // Not needed
  static void from_string(const char Str[], subject_type &Obj); // Not needed
  static std::string to_string(subject_type Obj)
	{ return pqxx::test::list_result_iterator(Obj); }
};


// Support string conversion on vector<string> for debug output.
template<> struct string_traits<std::vector<std::string> >
{
  typedef std::vector<std::string> subject_type;
  static const char *name() { return "vector<string>"; }
  static bool has_null() { return false; }
  static bool is_null(subject_type) { return false; }
  static subject_type null(); // Not needed
  static void from_string(const char Str[], subject_type &Obj); // Not needed
  static std::string to_string(const subject_type &Obj)
				 { return separated_list("; ", Obj); }
};
} // namespace pqxx
