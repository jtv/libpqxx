/// Libpqxx test runner.
/** This is the main program responsible for running libpqxx's test suite.
 * It is only really needed for developing libpqxx itself, though when you
 * build libpqxx it's definitely a good idea to build the test suite and run
 * this program to verify that everything works well in your specific
 * environment.
 *
 * Usage:
 *   runner [-j<jobs>|--jobs=<jobs>]
 *          [-s<seed>|--seed=<seed>]
 *          [test function...]
 *
 * The -j option dictates the number of parallel threads that will run the
 * tests.  I get most of the performance benefits from the parallelism by
 * setting this to 4; anything beyond that is probably overkill.
 *
 * The -s option sets an initial random seed, for reproducible or randomised
 * test runs.  If set to zero (the default), will randomise the seed.  Random
 * values in tests will differ for almost any two runs with this setting.
 */
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <new>
#include <random>
#include <semaphore>
#include <stdexcept>
#include <string>
#include <thread>

#include <pqxx/transaction>

#include "helpers.hxx"

namespace pqxx::test
{
test_failure::test_failure(std::string const &desc, sl loc) :
        std::logic_error{desc}, m_loc{loc}
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
void suite::register_test(std::string_view name, testfunc func) noexcept
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
std::string describe_failure(std::string_view test, EXCEPTION const &err)
{
  std::string summary;
  char const *const msg{err.what()};
  if constexpr (requires { err.location(); })
  {
    std::string const loc{pqxx::source_loc(err.location())};

    if ((msg != nullptr) and (msg[0] != '\0'))
      summary = std::format("[{}] ({}): {}", err.name(), loc, msg);
    else
      summary = std::format("{} ({})", err.name(), loc);
  }
  else
  {
    if ((msg != nullptr) and (msg[0] != '\0'))
      summary = msg;
    else
      summary = "unknown error";
  }

  if constexpr (requires { err.query(); })
  {
    if (not std::empty(err.query()))
      summary = std::format("{} -- {}\nQuery: {}", test, summary, err.query());
  }
  else
  {
    summary = std::format("{} -- {}", test, summary);
  }

#if defined(PQXX_HAVE_STACKTRACE)
  // TODO: Control this with command-line option, to avoid huge outputs.
  if constexpr (requires { err.trace(); })
    if (err.trace())
      summary = std::format("{}\nTraceback:\n{}", summary, *err.trace());
#endif

  return summary;
}


/// Produce a human-readable string describing an error from a test.
/** This is for the case where we don't have a meaningful exception object.
 */
std::string describe_failure(std::string_view desc, std::string_view test)
{
  return std::format("{} -- {}", test, desc);
}


/// Run one test.  Return optional failure message.
std::optional<std::string> run_test(
  std::string_view name, pqxx::test::testfunc func, pqxx::test::context &tctx)
{
  try
  {
    func(tctx);
  }
  catch (pqxx::test::test_failure const &e)
  {
    return describe_failure(name, e);
  }
  catch (pqxx::failure const &e)
  {
    return describe_failure(name, e);
  }
  catch (std::exception const &e)
  {
    return describe_failure(name, e);
  }
  catch (...)
  {
    // C++26: Can introspection give us more details?
    return describe_failure("Unknown exception", name);
  }
  return {};
}


/// Maximum allowed number of concurrent tests.
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
          m_here{m_tests.begin()},
          m_capacity{jobs}
  {
    assert(m_jobs <= max_jobs);
  }

  ~dispatcher() = default;

  /// Start the worker threads.
  /** This class does not manage the pool of workers.  But the workers can't
   * run any tests until this function is called.
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

  dispatcher(dispatcher const &) = delete;
  dispatcher &operator=(dispatcher const &) = delete;
  dispatcher(dispatcher &&) = delete;
  dispatcher &operator=(dispatcher &&) = delete;

private:
  std::ptrdiff_t m_jobs;
  std::vector<std::string_view> const m_tests;
  std::vector<std::string_view>::const_iterator m_here;
  std::counting_semaphore<max_jobs> m_capacity;
  std::mutex m_progress;
};


/// Work through tests waiting to be executed.  Runs in each worker thread.
void execute(
  dispatcher &disp,
  std::map<std::string_view, pqxx::test::testfunc> const &all_tests,
  std::mutex &fail_lock, std::vector<std::string> &failures,
  std::size_t random_seed)
{
  // Thread-local test context.
  pqxx::test::context tctx{random_seed};

  // Execute tests while there are any left to do.
  for (std::string_view test{disp.next()}; not std::empty(test);
       test = disp.next())
  {
    tctx.seed(test);
    auto const func{all_tests.at(test)};
    auto const msg{run_test(test, func, tctx)};
    if (msg.has_value())
    {
      std::lock_guard<std::mutex> const l{fail_lock};
      std::cerr << *msg << '\n';
      failures.emplace_back(*msg);
    }
  }
}


/// Exception class: user requested help output, exit cleanly.
struct help_exit final : std::exception
{};


/// Parsed command line.
struct options final
{
  /// Test functions to run.  If empty, run all.
  std::vector<std::string_view> tests;

  /// Number of parallel test threads.
  /** On my laptop with 8 physical cores, 4 workers give me about 95% of the
   * performance I get from 300 workers.  That can change radically though:
   * right now there are just a few "negative tests" holding things up by
   * waiting for a few seconds to check that something doesn't happen.
   */
  std::ptrdiff_t jobs{4};

  /// Random seed for randomised values in tests.
  /** If seed is zero (the default), we'll use something variable.
   */
  std::size_t seed{0};
};


options parse_command_line(int argc, char const *argv[])
{
  options opts;

  // Is the command-line parser in a state where it needs an argument to the
  // preceding "--jobs" option, or "--seed" option, respectively?
  bool want_jobs{false}, want_seed{false};

  // TODO: Store jobs & seed as strings, parse afterwards, once.
  // Parse command line.
  for (int arg{1}; arg < argc; ++arg)
  {
    std::string_view const elt{argv[arg]};
    if ((elt == "--help") or (elt == "-h"))
    {
      std::cout << "Test runner for libpqxx.\n"
                   "Usage: "
                << argv[0]
                << " [ -j <jobs> | --jobs=<jobs> ] [ test_function ... ]\n";
      throw help_exit{};
    }
    else if (want_jobs)
    {
      // Expecting an argument to the "jobs" option.
      opts.jobs = pqxx::from_string<std::ptrdiff_t>(elt);
      want_jobs = false;
    }
    else if (want_seed)
    {
      opts.seed = pqxx::from_string<std::size_t>(elt);
      want_seed = false;
    }
    else if ((elt == "-j") or (elt == "--jobs"))
    {
      // The "jobs" option, where the actual number is in the next element.
      want_jobs = true;
    }
    else if (elt.starts_with("-j"))
    {
      // Short-form "jobs" option, with a number attached.
      opts.jobs = pqxx::from_string<std::ptrdiff_t>(elt.substr(2));
    }
    else if (elt.starts_with("--jobs="))
    {
      // Long-form "jobs" option, with a number attached.
      opts.jobs = pqxx::from_string<std::ptrdiff_t>(elt.substr(7));
    }
    else if ((elt == "-s") or (elt == "--seed"))
    {
      // The "seed" option, where the actual number is in the next element.
      want_seed = true;
    }
    else if (elt.starts_with("-s"))
    {
      opts.seed = pqxx::from_string<std::size_t>(elt.substr(2));
    }
    else if (elt.starts_with("--seed="))
    {
      opts.seed = pqxx::from_string<std::size_t>(elt.substr(7));
    }
    else
    {
      // A test name.
      opts.tests.emplace_back(elt);
    }
  }
  if (want_jobs)
    throw std::runtime_error{"The jobs option needs a numeric argument."};
  if (want_seed)
    throw std::runtime_error{"The seed option needs a numeric argument."};
  if (opts.jobs <= 0)
    throw std::runtime_error{"Number of parallel jobs must be at least 1."};

  return opts;
}


/// Choose a random seed: either the given one, or if zero, a random one.
std::size_t get_random_seed(std::size_t seed_opt)
{
  if (seed_opt == 0u)
    return std::random_device{}();
  else
    return seed_opt;
}
} // namespace


int main(int argc, char const *argv[])
{
  try
  {
    auto opts{parse_command_line(argc, argv)};
    auto const seed{get_random_seed(opts.seed)};
    std::cout << "Random seed: " << seed << '\n';

    auto const all_tests{pqxx::test::suite::gather()};
    if (std::empty(opts.tests))
    {
      // Caller didn't pass any test names on the command line.  Run all.
      for (auto const &[name, _] : all_tests) opts.tests.emplace_back(name);
    }
    else
    {
      for (auto const name : opts.tests)
        if (not all_tests.contains(name))
        {
          std::cerr << "Unknown test: " << name << ".\n";
          return 2;
        }
    }
    auto const test_count{std::size(opts.tests)};

    std::mutex fail_lock;
    std::vector<std::string> failures;

    dispatcher disp{opts.jobs, std::move(opts.tests)};

    std::vector<std::thread> pool;
    pool.reserve(static_cast<std::size_t>(opts.jobs));
    for (std::ptrdiff_t j{0}; j < opts.jobs; ++j)
      pool.emplace_back(
        execute, std::ref(disp), std::cref(all_tests), std::ref(fail_lock),
        std::ref(failures), seed);

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
  catch (help_exit const &)
  {
    return 0;
  }
  catch (std::exception const &e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
