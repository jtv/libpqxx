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

#include "test_helpers.hxx"

namespace pqxx::test
{
#if defined(PQXX_HAVE_SOURCE_LOCATION)
test_failure::test_failure(std::string const &desc, std::source_location loc) :
        std::logic_error{desc}, m_loc{loc}
{}
#else
test_failure::test_failure(
  std::string const &ffile, int fline, std::string const &desc) :
        std::logic_error(desc), m_file(ffile), m_line(fline)
{}
#endif

test_failure::~test_failure() noexcept = default;


/// Drop table, if it exists.
inline void drop_table(transaction_base &t, std::string const &table)
{
  t.exec("DROP TABLE IF EXISTS " + table);
}


[[noreturn]] void check_notreached(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  std::string desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
)
{
  throw test_failure{
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
    file, line,
#endif
    desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
    ,
    loc
#endif
  };
}


void check(
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
  char const file[], int line,
#endif
  bool condition, char const text[], std::string const &desc
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
)
{
  if (not condition)
    throw test_failure{
#if !defined(PQXX_HAVE_SOURCE_LOCATION)
      file, line,
#endif
      desc + " (failed expression: " + text + ")"
#if defined(PQXX_HAVE_SOURCE_LOCATION)
      ,
      loc
#endif
    };
}


void expected_exception(std::string const &message)
{
  std::cout << "(Expected) " << message << std::endl;
}


std::string list_row(row Obj)
{
  return separated_list(
    ", ", std::begin(Obj), std::end(Obj),
    [](pqxx::field const &f) { return f.view(); });
}


std::string list_result(result Obj)
{
  if (std::empty(Obj))
    return "<empty>";
  return "{" +
         separated_list(
           "}\n{", std::begin(Obj), std::end(Obj),
           [](row const &r) { return list_row(r); }) +
         "}";
}


std::string list_result_iterator(result::const_iterator const &Obj)
{
  return "<iterator at " + to_string(Obj.rownumber()) + ">";
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


namespace
{
std::vector<std::string_view> *all_test_names{nullptr};
std::vector<pqxx::test::testfunc> *all_test_funcs{nullptr};
} // namespace


namespace pqxx::test
{
void register_test(char const name[], pqxx::test::testfunc func)
{
  if (all_test_names == nullptr)
  {
    assert(all_test_funcs == nullptr);
    all_test_names = new std::vector<std::string_view>;
    all_test_funcs = new std::vector<pqxx::test::testfunc>;
    all_test_names->reserve(1000);
    all_test_funcs->reserve(1000);
  }
  all_test_names->emplace_back(name);
  all_test_funcs->emplace_back(func);
}
} // namespace pqxx::test


int main(int argc, char const *argv[])
{
  // TODO: Accept multiple names.
  std::string_view test_name;
  if (argc > 1)
    test_name = argv[1];

  auto const num_tests{std::size(*all_test_names)};
  std::map<std::string_view, pqxx::test::testfunc> all_tests;
  for (std::size_t idx{0}; idx < num_tests; ++idx)
  {
    auto const name{all_test_names->at(idx)};
    // C++20: Use std::map::contains().
    assert(all_tests.find(name) == std::end(all_tests));
    all_tests.emplace(name, all_test_funcs->at(idx));
  }

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
#if defined(PQXX_HAVE_SOURCE_LOCATION)
        std::cerr << "(";
        std::cerr << e.location.file_name() << ':' << e.location.line();
        if (not name.empty())
          std::cerr << " in " << name;
        std::cerr << ")\n";
#endif
        success = true;
        --test_count;
      }
      catch (pqxx::sql_error const &e)
      {
        std::cerr << "SQL error: " << e.what() << '\n';
#if defined(PQXX_HAVE_SOURCE_LOCATION)
        std::cerr << "(";
        std::cerr << e.location.file_name() << ':' << e.location.line();
        if (not name.empty())
          std::cerr << " in " << name;
        std::cerr << ")\n";
#endif
        std::cerr << "Query was: " << e.query() << '\n';
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
