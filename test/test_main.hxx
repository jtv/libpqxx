/* main() definition for libpqxx test runners.
 */
#include <cassert>
#include <iostream>
#include <list>
#include <new>
#include <stdexcept>

#include "test_helpers.hxx"


using namespace pqxx;
using namespace pqxx::test;


namespace
{
std::string deref_field(const field &f)
{
  return f.c_str();
}


} // namespace


namespace pqxx
{
namespace test
{
test_failure::test_failure(
	const std::string &ffile,
	int fline,
	const std::string &desc) :
  logic_error(desc),
  m_file(ffile),
  m_line(fline)
{
}

test_failure::~test_failure() noexcept {}


/// Drop table, if it exists.
void drop_table(transaction_base &t, const std::string &table)
{
  t.exec("DROP TABLE IF EXISTS " + table);
}


[[noreturn]] void check_notreached(
	const char file[],
	int line,
	std::string desc)
{
  throw test_failure(file, line, desc);
}


void check(
	const char file[],
	int line,
	bool condition,
	const char text[],
	std::string desc)
{
  if (not condition)
    throw test_failure(
	file,
	line,
	desc + " (failed expression: " + text + ")");
}


void expected_exception(const std::string &message)
{
  std::cout << "(Expected) " << message << std::endl;
}


std::string list_row(row Obj)
{
  return separated_list(", ", Obj.begin(), Obj.end(), deref_field);
}


std::string list_result(result Obj)
{
  if (Obj.empty()) return "<empty>";
  return "{" + separated_list("}\n{", Obj) + "}";
}


std::string list_result_iterator(result::const_iterator Obj)
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
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (1978, 'bloody\t\tcold')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (99, '')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (2002, 'libpqxx')");
  t.exec(
	"INSERT INTO pqxxevents(year, event) "
	"VALUES (1989, 'Ode an die Freiheit')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (2001, 'New millennium')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (1974, '')");
  t.exec("INSERT INTO pqxxevents(year, event) VALUES (97, 'Asian crisis')");
  t.exec(
	"INSERT INTO pqxxevents(year, event) VALUES (2001, 'A Space Odyssey')");
}
} // namespace pqxx::test
} // namespace pqxx


namespace
{
std::map<const char *, testfunc> *all_tests = nullptr;
} // namespace


namespace pqxx
{
namespace test
{
void register_test(const char name[], testfunc func)
{
  if (all_tests == nullptr)
  {
    all_tests = new std::map<const char *, testfunc>();
  }
  else
  {
    assert(all_tests->find(name) == all_tests->end());
  }
  (*all_tests)[name] = func;
}
} // namespace pqxx::test
} // namespace pqxx

int main(int, const char *argv[])
{
  const char *const test_name = argv[1];

  int test_count = 0;
  std::list<std::string> failed;
  for (const auto &i: *all_tests)
    if (test_name == nullptr or std::string{test_name} == std::string{i.first})
    {
      std::cout << std::endl << "Running: " << i.first << std::endl;

      bool success = false;
      try
      {
        i.second();
        success = true;
      }
      catch (const test_failure &e)
      {
        std::cerr << "Test failure in " + e.file() + " line " +
	    to_string(e.line()) << ": " << e.what() << std::endl;
      }
      catch (const std::bad_alloc &)
      {
        std::cerr << "Out of memory!" << std::endl;
      }
      catch (const feature_not_supported &e)
      {
        std::cerr
		<< "Not testing unsupported feature: " << e.what() << std::endl;
        success = true;
        --test_count;
      }
      catch (const sql_error &e)
      {
        std::cerr
		<< "SQL error: " << e.what() << std::endl
		<< "Query was: " << e.query() << std::endl;
      }
      catch (const std::exception &e)
      {
        std::cerr << "Exception: " << e.what() << std::endl;
      }
      catch (...)
      {
        std::cerr << "Unknown exception" << std::endl;
      }

      if (not success)
      {
        std::cerr << "FAILED: " << i.first << std::endl;
        failed.push_back(i.first);
      }
      ++test_count;
    }

  std::cout << "Ran " << test_count << " test(s)." << std::endl;

  if (not failed.empty())
  {
    std::cerr << "*** " << failed.size() << " test(s) failed: ***" << std::endl;
    for (const auto &i: failed) std::cerr << "\t" << i << std::endl;
  }

  return int(failed.size());
}
