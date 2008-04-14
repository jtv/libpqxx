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


// Base class for libpqxx tests.  Sets up a connection and transaction.
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
  {}

  void operator()() { m_func(m_conn, m_trans); }

private:
  CONNECTION m_conn;
  TRANSACTION m_trans;
  testfunc &m_func;
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
		"threw='" + e.what() + ")"); \
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

} // namespace pqxx
} // namespace test

