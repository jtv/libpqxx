#include <iostream>
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


// Verify that variable has the expected value.
#define PQXX_CHECK_EQUAL(actual, expected, desc) \
	if (!(expected == actual)) throw pqxx::test::test_failure( \
		__FILE__, \
		__LINE__, \
		PGSTD::string(desc) + " " \
		"(" #actual " <> " #expected ": " + \
		"expected=" + to_string(expected) + ", " \
		"actual=" + to_string(actual) + ")");

// Verify that "action" throws "exception_type."
#define PQXX_CHECK_THROWS(action, exception_type, desc) \
	{ \
	  bool pqxx_check_throws_failed = true; \
	  try \
	  { \
	    action ; \
	    pqxx_check_throws_failed = false; \
	  } \
	  catch (const exception_type &) {} \
	  catch (const PGSTD::exception &e) \
	  { \
	    throw pqxx::test::test_failure( \
		__FILE__, \
		__LINE__, \
		PGSTD::string(desc) + " " \
		"(" #action ": " \
		"expected=" #exception_type ", " \
		"threw='" + e.what() + "')"); \
	  } \
	  catch (...) \
	  { \
	    throw pqxx::test::test_failure( \
		__FILE__, \
		__LINE__, \
		PGSTD::string(desc) + " " \
		"(" #action ": " "unexpected exception)"); \
	  } \
	  if (!pqxx_check_throws_failed) throw pqxx::test::test_failure( \
		__FILE__, \
		__LINE__, \
		PGSTD::string(desc) + " " \
		"(" #action ": " \
		"expected=" #exception_type ", " \
		"did not throw)"); \
	}

} // namespace test
} // namespace pqxx

