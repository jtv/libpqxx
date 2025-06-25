/* libpqxx test runner.
 */
#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <new>
#include <stdexcept>
#include <string>

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


int main(int argc, char const *argv[])
{
  // TODO: Accept multiple names.
  std::string_view test_name;
  if (argc > 1)
    test_name = argv[1];

  auto const all_tests{pqxx::test::suite::gather()};
  int test_count = 0;
  std::vector<std::string> failed;
  for (auto const [name, func] : all_tests)
    if ((test_name.empty()) or (name == test_name))
    {
      std::cout << std::endl << "Running: " << name << '\n';

      bool success{false};
      try
      {
        func();
        success = true;
      }
      catch (pqxx::test::test_failure const &e)
      {
        std::cerr << "Test failure in " << e.file() << " line "
                  << pqxx::to_string(e.line()) << ": " << e.what() << '\n';
      }
      catch (std::bad_alloc const &)
      {
        std::cerr << "Out of memory!\n";
      }
      catch (pqxx::feature_not_supported const &e)
      {
        std::cerr << "Not testing unsupported feature: " << e.what() << '\n';
        std::cerr << "(";
        std::cerr << e.location.file_name() << ':' << e.location.line();
        if (not name.empty())
          std::cerr << " in " << name;
        std::cerr << ")\n";
        success = true;
        --test_count;
      }
      catch (pqxx::sql_error const &e)
      {
        std::cerr << "SQL error: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location)
                  << ")\nQuery was: " << e.query() << '\n';
      }
      catch (pqxx::internal_error const &e)
      {
        std::cerr << "Internal error: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location) << ")\n";
      }
      catch (pqxx::usage_error const &e)
      {
        std::cerr << "Usage error: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location) << ")\n";
      }
      catch (pqxx::conversion_error const &e)
      {
        std::cerr << "Conversion error: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location) << ")\n";
      }
      catch (pqxx::argument_error const &e)
      {
        std::cerr << "Argument error: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location) << ")\n";
      }
      catch (pqxx::range_error const &e)
      {
        std::cerr << "Range error: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location) << ")\n";
      }
      catch (pqxx::failure const &e)
      {
        std::cerr << "Failure: " << e.what() << "\n("
                  << pqxx::internal::source_loc(e.location) << ")\n";
      }
      catch (std::exception const &e)
      {
        std::cerr << "Exception: " << e.what() << '\n';
      }
      catch (...)
      {
        std::cerr << "Unknown exception.\n";
      }

      if (not success)
      {
        std::cerr << "FAILED: " << name << '\n';
        failed.emplace_back(name);
      }
      ++test_count;
    }

  std::cout << "Ran " << test_count << " test(s).\n";

  if (not std::empty(failed))
  {
    std::cerr << "*** " << std::size(failed) << " test(s) failed: ***\n";
    for (auto const &i : failed) std::cerr << "\t" << i << '\n';
  }

  return int(std::size(failed));
}
