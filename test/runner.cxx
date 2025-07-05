/* libpqxx test runner.
 */
#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <new>
#include <semaphore>
#include <stdexcept>
#include <string>

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
/// Produce a human-readable string describing a test failure.
std::string describe_failure(
  std::string_view desc, std::string_view test, std::string_view msg = "",
  std::optional<pqxx::sl> loc = {})
{
  std::string summary;
  if (loc.has_value())
  {
    if (std::empty(msg))
      summary = std::format("{} ({})", desc, pqxx::internal::source_loc(*loc));
    else
      summary = std::format(
        "{} ({}): {}", desc, pqxx::internal::source_loc(*loc), msg);
  }
  else
  {
    if (std::empty(msg))
      summary = desc;
    else
      summary = std::format("{}: {}", desc, msg);
  }
  return std::format("{} -- {}", test, summary);
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
    return describe_failure("Failed", name, e.what(), e.location);
  }
  catch (std::bad_alloc const &)
  {
    return describe_failure("Out of memory", name);
  }
  catch (pqxx::failure const &e)
  {
    return describe_failure("SQL error", name, e.what(), e.location);
  }
  catch (pqxx::internal_error const &e)
  {
    return describe_failure("Internal error", name, e.what(), e.location);
  }
  catch (pqxx::usage_error const &e)
  {
    return describe_failure("Usage error", name, e.what(), e.location);
  }
  catch (pqxx::conversion_error const &e)
  {
    return describe_failure("Conversion error", name, e.what(), e.location);
  }
  catch (pqxx::argument_error const &e)
  {
    return describe_failure("Argument error", name, e.what(), e.location);
  }
  catch (pqxx::range_error const &e)
  {
    return describe_failure("Range error", name, e.what(), e.location);
  }
  catch (std::exception const &e)
  {
    return describe_failure("Exception", name, e.what());
  }
  catch (...)
  {
    return describe_failure("Unknown exception", name);
  }
  return {};
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
    for (auto const [name, _] : all_tests) tests.emplace_back(name);
  }

  int test_count = 0;
  std::vector<std::string> failures;
  for (auto const name : tests)
  {
    if (not all_tests.contains(name))
    {
      std::cerr << "** Unknown test: " << name << " **\n";
      return 1;
    }
    std::cout << std::endl << "Running: " << name << '\n';

    auto const func{all_tests.at(name)};
    auto const msg{run_test(name, func)};
    if (msg.has_value())
    {
      std::cerr << *msg << '\n';
      failures.emplace_back(*msg);
    }
    ++test_count;
  }

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
    for (auto const &i : failures) std::cerr << "\t" << i << '\n';
    return 1;
  }
}
