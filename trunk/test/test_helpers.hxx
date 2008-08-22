#include <iostream>
#include <map>
#include <new>
#include <stdexcept>

#include <pqxx/pqxx>


namespace pqxx
{
namespace test
{
class test_failure : public PGSTD::logic_error
{
  const PGSTD::string m_file;
  int m_line;

public:
  test_failure(
	const PGSTD::string &ffile, int fline, const PGSTD::string &desc) :
    logic_error(desc),
    m_file(ffile),
    m_line(fline)
  {}

  ~test_failure() throw () {}

  const PGSTD::string &file() const throw () { return m_file; }
  int line() const throw () { return m_line; }
};


/// Does this backend have generate_series()?
inline bool have_generate_series(const connection_base &c)
{
  return c.server_version() >= 80000;
}

/// For backends that don't have generate_series(): sequence of ints
/** If the backend lacks generate_series(), prepares a temp table called
 * series" containing given range of numbers (including lowest and highest).
 *
 * Use select_series() to construct a query selecting a range of numbers.  For
 * the workaround on older backends to work, the ranges of numbers passed to
 * select_series() must be subsets of the range passed here.
 */
inline void prepare_series(transaction_base &t, int lowest, int highest)
{
  connection_base &conn = t.conn();
  // Don't do this for nullconnections, so nullconnection tests can run.
  if (conn.is_open() && !have_generate_series(conn))
  {
    t.exec("CREATE TEMP TABLE series(x integer)");
    for (int x=lowest; x <= highest; ++x)
      t.exec("INSERT INTO series(x) VALUES (" + to_string(x) + ")");
  }
}

/// Generate query selecting series of numbers from lowest to highest, inclusive
/** Needs to see connection object to determine whether the backend supports
 * generate_series().
 */
inline
PGSTD::string select_series(connection_base &conn, int lowest, int highest)
{
  if (have_generate_series(conn))
    return
	"SELECT generate_series(" +
	to_string(lowest) + ", " + to_string(highest) + ")";

  return
	"SELECT x FROM series "
	"WHERE "
	  "x >= " + to_string(lowest) + " AND "
	  "x <= " + to_string(highest) + " "
	"ORDER BY x";
}


class base_test;
typedef PGSTD::map<PGSTD::string, base_test *> test_map;
const test_map &register_test(base_test *);


/// Base class for test cases.
class base_test
{
public:
  typedef void (*testfunc)(connection_base &, transaction_base &);
  base_test(const PGSTD::string &tname, testfunc func) :
	m_name(tname),
	m_func(func)
  {
    register_test(this);
  }
  virtual int run() =0;
  virtual ~base_test() =0;
  const PGSTD::string &name() const throw () { return m_name; }
private:
  PGSTD::string m_name;
protected:
  testfunc m_func;
};


/// Runner class for libpqxx tests.  Sets up a connection and transaction.
template<typename CONNECTION=connection, typename TRANSACTION=work>
class test_case : public base_test
{
public:
  // func takes connection and transaction as arguments.
  test_case(const PGSTD::string &tname, testfunc func) :
    base_test(tname, func),
    m_conn(),
    m_trans(m_conn, tname)
  {
    // Workaround for older backend versions that lack generate_series().
    prepare_series(m_trans, 0, 100);
  }

  ~test_case() {}

  // Run test, catching errors & returning Unix-style success value
  virtual int run()
  {
    try
    {
      m_func(m_conn, m_trans);
    }
    catch (const test_failure &e)
    {
      PGSTD::cerr << "Test failure in " + e.file() + " line " + 
	  to_string(e.line()) << ": " << e.what() << PGSTD::endl;
      return 1;
    }
    catch (const PGSTD::bad_alloc &)
    {
      PGSTD::cerr << "Out of memory!" << PGSTD::endl;
      return 50;
    }
    catch (const sql_error &e)
    {
      PGSTD::cerr << "SQL error: " << e.what() << PGSTD::endl
           << "Query was: " << e.query() << PGSTD::endl;
      return 1;
    }
    catch (const PGSTD::exception &e)
    {
      PGSTD::cerr << "Exception: " << e.what() << PGSTD::endl;
      return 2;
    }
    catch (...)
    {
      PGSTD::cerr << "Unknown exception" << PGSTD::endl;
      return 100;
    }

    return 0;
  }

private:
  CONNECTION m_conn;
  TRANSACTION m_trans;
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


#define PQXX_RUN_TESTS \
	namespace pqxx \
	{ \
	namespace test \
	{ \
	base_test::~base_test() {} \
	const test_map &register_test(base_test *tc) \
	{ \
	  static test_map tests; \
	  if (tc) tests[tc->name()] = tc; \
	  return tests; \
	} \
	} \
	} \
	int main() \
	{ \
	  pqxx::test::run_tests(); \
	}

inline void run_tests()
{
  const test_map &tests = register_test(NULL);
  for (test_map::const_iterator i = tests.begin(); i != tests.end(); ++i)
  {
    PGSTD::cout << "Running: " << i->first << PGSTD::endl;
    i->second->run();
  }
}


// Unconditional test failure.
#define PQXX_CHECK_NOTREACHED(desc) \
	pqxx::test::check_notreached(__FILE__, __LINE__, (desc))
inline void check_notreached(const char file[], int line, PGSTD::string desc)
{
  throw test_failure(file, line, desc);
}

// Verify that a condition is met, similar to assert()
#define PQXX_CHECK(condition, desc) \
	pqxx::test::check(__FILE__, __LINE__, (condition), #condition, (desc))
inline void check(
	const char file[],
	int line,
	bool condition,
	const char text[], 
	PGSTD::string desc)
{
  if (!condition)
    throw test_failure(
	file,
	line,
	desc + " (failed expression: " + text + ")");
}

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
	PGSTD::string desc)
{
  if (expected == actual) return;
  const PGSTD::string fulldesc =
	desc + " (" + actual_text + " <> " + expected_text + ": "
	"expected=" + to_string(expected) + ", "
	"actual=" + to_string(actual) + ")";
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
	PGSTD::string desc)
{
  if (value1 != value2) return;
  const PGSTD::string fulldesc =
	desc + " (" + text1 + " == " + text2 + ": "
	"both are " + to_string(value2) + ")";
  throw test_failure(file, line, fulldesc);
}


struct failure_to_fail {};

// Verify that "action" throws "exception_type."
#define PQXX_CHECK_THROWS(action, exception_type, desc) \
	do { \
	  try \
	  { \
	    action ; \
	    throw pqxx::test::failure_to_fail(); \
	  } \
	  catch (const pqxx::test::failure_to_fail &) \
	  { \
	    PQXX_CHECK_NOTREACHED( \
		PGSTD::string(desc) + \
		" (\"" #action "\" did not throw " #exception_type ")"); \
	  } \
	  catch (const exception_type &e) {} \
	  catch (...) \
	  { \
	    PQXX_CHECK_NOTREACHED( \
		PGSTD::string(desc) + \
		" (\"" #action "\" " \
		"threw exception other than " #exception_type ")"); \
	  } \
	} while (false)

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
	const PGSTD::string &desc)
{
  if (!(lower < upper))
  {
    const PGSTD::string fulldesc =
	desc + " ("
	"bounds allow no values: " +
	lower_text + " >= " + upper_text + ": "
	"lower=" + to_string(lower) + ", "
	"upper=" + to_string(upper) + ", "
	"value=" + to_string(value) + ")";
    throw test_failure(file, line, fulldesc);
  }

  if (value < lower)
  {
    const PGSTD::string fulldesc =
	desc + " (" +
	text + " is below lower bound " + lower_text + ": " +
	to_string(value) + " < " + to_string(lower) + ")";
    throw test_failure(file, line, fulldesc);
  }

  if (!(value < upper))
  {
    const PGSTD::string fulldesc =
	desc + " (" +
	text + " is not below upper bound " + upper_text + ": " +
	to_string(value) + " >= " + to_string(upper) + ")";
    throw test_failure(file, line, fulldesc);
  }
}
} // namespace test


namespace
{
PGSTD::string deref_field(const pqxx::result::field &f) { return f.c_str(); }
} // namespace


// Support string conversion on result rows for debug output.
template<> struct string_traits<result::tuple>
{
  static const char *name() { return "pqxx::result::tuple"; }
  static bool has_null() { return false; }
  static bool is_null(result::tuple) { return false; }
  static result null(); // Not needed
  static void from_string(const char Str[], result &Obj); // Not needed
  static PGSTD::string to_string(result::tuple Obj)
  {
    return separated_list(", ", Obj.begin(), Obj.end(), deref_field);
  }
};

// Support string conversion on result objects for debug output.
template<> struct string_traits<result>
{
  static const char *name() { return "pqxx::result"; }
  static bool has_null() { return true; }
  static bool is_null(result r) { return r.empty(); }
  static result null() { return result(); }
  static void from_string(const char Str[], result &Obj); // Not needed
  static PGSTD::string to_string(result Obj)
  {
    if (is_null(Obj)) return "<empty>";
    return "{" + separated_list("}\n{", Obj) + "}";
  }
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
  static PGSTD::string to_string(subject_type Obj)
  {
    return "<iterator at " + pqxx::to_string(Obj.rownumber()) + ">";
  }
};


// Support string conversion on vector<string> for debug output.
template<> struct string_traits<PGSTD::vector<PGSTD::string> >
{
  typedef PGSTD::vector<PGSTD::string> subject_type;
  static const char *name() { return "vector<string>"; }
  static bool has_null() { return false; }
  static bool is_null(subject_type) { return false; }
  static subject_type null(); // Not needed
  static void from_string(const char Str[], subject_type &Obj); // Not needed
  static PGSTD::string to_string(const subject_type &Obj)
				 { return separated_list("; ", Obj); }
};
} // namespace pqxx


