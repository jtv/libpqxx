/* libpqxx test runner.
 */
#include <cassert>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <new>
#include <semaphore>
#include <stdexcept>
#include <string>
#include <thread>

#include <pqxx/transaction>

#include "helpers.hxx"

namespace pqxx::test
{
test_failure::test_failure(std::string const &desc, sl loc) :
        std::logic_error{desc}, location{loc}
{}

test_failure::~test_failure() noexcept = default;


/// Drop table, if it exists.
inline void drop_table(transaction_base &t, std::string const &table, sl loc)
{
  t.exec("DROP TABLE IF EXISTS " + table, loc);
}


[[noreturn]] void check_notreached(std::string const &desc, sl loc)
{
  throw test_failure{desc, loc};
}


void check(bool condition, char const text[], std::string const &desc, sl loc)
{
  if (not condition)
    throw test_failure{
      std::format("{} (failed expression: '{}')", desc, text), loc};
}


void expected_exception(std::string const &message)
{
  std::cout << "(Expected) " << message << std::endl;
}


std::string list_row(row_ref Obj)
{
  return separated_list(
    ", ", std::begin(Obj), std::end(Obj),
    [](pqxx::row_ref::iterator f) { return f->view(); });
}


std::string list_row(row Obj)
{
  return separated_list(
    ", ", std::begin(Obj), std::end(Obj),
    [](pqxx::row_ref::iterator f) { return f->view(); });
}


std::string list_result(result Obj)
{
  if (std::empty(Obj))
    return "<empty>";
  return "{" +
         separated_list(
           "}\n{", std::begin(Obj), std::end(Obj),
           [](pqxx::result::iterator r) { return list_row(*r); }) +
         "}";
}


std::string list_result_iterator(result::const_iterator const &Obj)
{
  return "<iterator at " + to_string(Obj->row_number()) + ">";
}


void create_pqxxevents(transaction_base &t)
{
  t.exec(
    "CREATE TEMP TABLE pqxxevents(year integer, event varchar) "
    "ON COMMIT PRESERVE ROWS");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (71, 'jtv')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (38, 'time_t overflow')");
  t.exec(
    "INSERT INTO pqxxevents(year, event) VALUES (1, '''911'' WTC attack')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (81, 'C:\\>')");
  t.exec(
    "INSERT INTO pqxxevents(year, event) VALUES (1978, 'bloody\t\tcold')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (99, '')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (2002, 'libpqxx')");
  t.exec(
    "INSERT INTO pqxxevents(year, event) "
    "VALUES (1989, 'Ode an die Freiheit')");
  t.exec(
    "INSERT INTO pqxxevents(year, event) VALUES (2001, 'New millennium')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (1974, '')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (97, 'Asian crisis')");
  t.exec(
    "INSERT INTO pqxxevents(year, event) VALUES (2001, 'A Space Odyssey')");
}
} // namespace pqxx::test


namespace pqxx::test
{
void suite::register_test(char const name[], testfunc func) noexcept
{
  assert(s_num_tests < max_tests);
  s_names.at(s_num_tests) = name;
  s_funcs.at(s_num_tests) = func;
  ++s_num_tests;
}

std::map<std::string_view, testfunc> suite::gather()
{
  std::map<std::string_view, testfunc> all_tests;
  for (std::size_t idx{0}; idx < s_num_tests; ++idx)
  {
    auto const name{s_names.at(idx)};
    assert(not all_tests.contains(name));
    auto const func{s_funcs.at(idx)};

    // This could happen if the internal arrays get constructed after we've
    // already written tests into them.
    assert(func != nullptr);
    all_tests.emplace(name, func);
  }

  return all_tests;
}


// This needs to get zero-initialised, not constructed in the usual way.
// It's a hack, reliant on an implementation detail.  But is there a better
// way?  To my knowledge there's no way in the standard to enforce an order on
// the initialisation of this class' internal state on the one hand, and that
// of the test registrations on the other.
constinit std::size_t suite::s_num_tests{0};


// These need to get zero-initialised as well, not constructed.
// Otherwise, construction will overwrite the values already written, which
// most likely will crash the test suite (as test function pointers get
// initialised as null pointers after function pointers had already been
// written there).
constinit std::array<std::string_view, max_tests> suite::s_names;
constinit std::array<testfunc, max_tests> suite::s_funcs{};
} // namespace pqxx::test


namespace
{
/// Produce a human-readable string describing an exception from a test.
/** A bunch of libpqxx exception types have a "location" field.  Use that if
 * available.
 */
template<typename EXCEPTION>
std::string describe_failure(
  std::string_view desc, std::string_view test, EXCEPTION const &err)
{
  std::string summary;
  char const *const msg{err.what()};
  if constexpr (requires { err.location; })
  {
    std::string const loc{pqxx::internal::source_loc(err.location)};

    if ((msg != nullptr) and (msg[0] != '\0'))
      summary = std::format("{} ({}): {}", desc, loc, msg);
    else
      summary = std::format("{} ({})", desc, loc, msg);
  }
  else
  {
    if ((msg != nullptr) and (msg[0] != '\0'))
      summary = std::format("{}: {}", desc, msg);
    else
      summary = desc;
  }

  return std::format("{} -- {}", test, summary);
}


/// Produce a human-readable string describing an error from a test.
/** This is for the case where we don't have a meaningful exception object.
 */
std::string describe_failure(std::string_view desc, std::string_view test)
{
  return std::format("{} -- {}", test, desc);
}


/// Run one test.  Return optional failure message.
std::optional<std::string>
run_test(std::string_view name, pqxx::test::testfunc func)
{
  try
  {
    func();
  }
  catch (pqxx::test::test_failure const &e)
  {
    return describe_failure("Failed", name, e);
  }
  catch (std::bad_alloc const &)
  {
    return describe_failure("Out of memory", name);
  }
  catch (pqxx::sql_error const &e)
  {
    return describe_failure("SQL error", name, e);
  }
  catch (pqxx::failure const &e)
  {
    return describe_failure("Failure", name, e);
  }
  catch (pqxx::internal_error const &e)
  {
    return describe_failure("Internal error", name, e);
  }
  catch (pqxx::usage_error const &e)
  {
    return describe_failure("Usage error", name, e);
  }
  catch (pqxx::conversion_error const &e)
  {
    return describe_failure("Conversion error", name, e);
  }
  catch (pqxx::argument_error const &e)
  {
    return describe_failure("Argument error", name, e);
  }
  catch (pqxx::range_error const &e)
  {
    return describe_failure("Range error", name, e);
  }
  catch (std::exception const &e)
  {
    return describe_failure("Exception", name, e);
  }
  catch (...)
  {
    // C++26: Can introspection give us more details?
    return describe_failure("Unknown exception", name);
  }
  return {};
}


/// Maximum allowed number of test concurrent tests.
/** No particular reason, except anything higher isn't likely to give us much
 * in the way of speedup while still increasing peak memory usage etc.
 */
inline constexpr std::ptrdiff_t max_jobs{255};


/// Dispatcher of individual tests.
/** Hands out the names of tests to be run to worker threads, on request, one
 * at a time.
 */
class dispatcher final
{
public:
  dispatcher(std::ptrdiff_t jobs, std::vector<std::string_view> &&tests) :
          m_jobs{jobs},
          m_tests{std::move(tests)},
          m_capacity{jobs},
          m_here{m_tests.begin()}
  {
    assert(m_jobs <= max_jobs);
  }

  /// Start the worker threads.
  /** This class does not manage the pool of workers.  But the workers can't
   * run any tests until this funtion is called.
   */
  void start() &
  {
    // Our capacity semaphore starts out with all slots "taken."  Release them
    // now to kick off the workers.
    for (auto i{m_jobs}; i > 0; --i) m_capacity.release();
  }

  /// Obtain a test name to run, or empty string if there are no more.
  /** Will give out each test exactly once.
   */
  [[nodiscard]] std::string_view next() & noexcept
  {
    std::string_view test{};
    m_capacity.acquire();
    std::lock_guard<std::mutex> const progress_lock{m_progress};
    if (m_here != m_tests.end())
      test = *m_here++;
    m_capacity.release();
    return test;
  }

private:
  std::ptrdiff_t m_jobs;
  std::vector<std::string_view> const m_tests;
  std::counting_semaphore<max_jobs> m_capacity;
  std::mutex m_progress;
  std::vector<std::string_view>::const_iterator m_here;
};


void execute(
  dispatcher &disp,
  std::map<std::string_view, pqxx::test::testfunc> const &all_tests,
  std::mutex &fail_lock, std::vector<std::string> &failures)
{
  for (std::string_view test{disp.next()}; not std::empty(test);
       test = disp.next())
  {
    auto const func{all_tests.at(test)};
    auto const msg{run_test(test, func)};
    if (msg.has_value())
    {
      std::lock_guard<std::mutex> const l{fail_lock};
      std::cerr << *msg << '\n';
      failures.emplace_back(*msg);
    }
  }
}
} // namespace


int main(int argc, char const *argv[])
{
  std::vector<std::string_view> tests;
  for (int arg{1}; arg < argc; ++arg) tests.emplace_back(argv[arg]);

  auto const all_tests{pqxx::test::suite::gather()};
  if (std::empty(tests))
  {
    // Caller didn't pass any test names on the command line.  Run all.
    for (auto const &[name, _] : all_tests) tests.emplace_back(name);
  }
  else
  {
    for (auto const name : tests)
      if (not all_tests.contains(name))
      {
        std::cerr << "Unknown test: " << name << ".\n";
        return 2;
      }
  }
  auto const test_count{std::size(tests)};

  std::mutex fail_lock;
  std::vector<std::string> failures;

  // Number of parallel worker threads for executing the tests.  On my laptop
  // with 8 physical cores, 4 workers give me about 95% of the performance I
  // get from 300 workers.  That can change radically though: right now there
  // are just a few "negative tests" holding things up by waiting for a few
  // seconds to check that something doesn't happen.
  std::ptrdiff_t const jobs{4};
  dispatcher disp{jobs, std::move(tests)};

  std::vector<std::thread> pool;
  pool.reserve(jobs);
  for (std::ptrdiff_t j{0}; j < jobs; ++j)
    pool.emplace_back(
      execute, std::ref(disp), std::cref(all_tests), std::ref(fail_lock),
      std::ref(failures));

  disp.start();

  for (auto &thread : pool) thread.join();

  if (test_count == 1)
    std::cout << "Ran " << test_count << " test.\n";
  else
    std::cout << "Ran " << test_count << " tests.\n";

  if (std::empty(failures))
  {
    std::cout << "Tests OK.\n";
    return 0;
  }
  else
  {
    std::cerr << "\n*** " << std::size(failures) << " test(s) failed: ***\n";

    // Lazy: each message starts with the test name, so mostly sorts by that.
    std::sort(failures.begin(), failures.end());
    for (auto const &i : failures) std::cerr << "- " << i << '\n';
    return 1;
  }
}
