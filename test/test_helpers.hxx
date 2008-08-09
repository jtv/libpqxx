#include <iostream>
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


// Run a libpqxx test function.
template<typename TESTFUNC>
inline int pqxxtest(TESTFUNC &func)
{
  try
  {
    func();
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
} // pqxxtest()


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
  if (!have_generate_series(t.conn()))
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
  if (pqxx::test::have_generate_series(conn))
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


/// Base class for libpqxx tests.  Sets up a connection and transaction.
template<typename CONNECTION=connection, typename TRANSACTION=work>
class TestCase
{
public:
  typedef void (*testfunc)(connection_base &, transaction_base &);

  // func takes connection and transaction as arguments.
  TestCase(const PGSTD::string &name, testfunc func) :
    m_conn(),
    m_trans(m_conn, name),
    m_func(func)
  {
    // Workaround for older backend versions that lack generate_series().
    prepare_series(m_trans, 0, 100);
  }

  void operator()() { m_func(m_conn, m_trans); }

private:
  CONNECTION m_conn;
  TRANSACTION m_trans;
  testfunc m_func;
};


// Unconditional test failure.
#define PQXX_CHECK_NOTREACHED(desc) \
	pqxx::test::check_notreached(__FILE__, __LINE__, (desc))
void check_notreached(const char file[], int line, PGSTD::string desc)
{
  throw test_failure(file, line, desc);
}

// Verify that a condition is met, similar to assert()
#define PQXX_CHECK(condition, desc) \
	pqxx::test::check(__FILE__, __LINE__, (condition), #condition, (desc))
void check(
	const char file[],
	int line,
	bool condition,
	const char text[], 
	PGSTD::string desc)
{
  if (!condition)
    throw test_failure(file, line, desc + " (failed expression: " + text + ")");
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
  throw pqxx::test::test_failure(file, line, fulldesc);
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
  throw pqxx::test::test_failure(file, line, fulldesc);
}

// Verify that "action" throws "exception_type."
#define PQXX_CHECK_THROWS(action, exception_type, desc) \
	do { \
	  try \
	  { \
	    action ; \
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

} // namespace test


namespace
{
PGSTD::string deref_field(const result::field &f) { return f.c_str(); }
} // namespace


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

    PGSTD::string out;
    for (result::const_iterator row = Obj.begin(); row != Obj.end(); ++row)
    {
      out += "{" +
	separated_list(", ", row.begin(), row.end(), deref_field) +
	"}";
    }
    return out;
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

