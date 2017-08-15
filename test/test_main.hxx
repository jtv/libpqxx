/* main() definition for libpqxx test runners.
 */
#include <iostream>
#include <list>
#include <new>
#include <stdexcept>

#include "test_helpers.hxx"


using namespace std;
using namespace pqxx;
using namespace pqxx::test;


namespace
{
string deref_field(const field &f)
{
  return f.c_str();
}


} // namespace


namespace pqxx
{
namespace test
{
test_failure::test_failure(const string &ffile, int fline, const string &desc) :
  logic_error(desc),
  m_file(ffile),
  m_line(fline)
{
}

test_failure::~test_failure() noexcept {}


base_test::base_test(const string &tname, testfunc func) :
  m_name(tname),
  m_func(func)
{
  register_test(this);
}


base_test::~base_test() {}


const test_map &register_test(base_test *tc)
{
  static test_map tests;
  if (tc) tests[tc->name()] = tc;
  return tests;
}


/// Drop table, if it exists.
void drop_table(transaction_base &t, const std::string &table)
{
  t.exec("DROP TABLE IF EXISTS " + table);
}


[[noreturn]] void check_notreached(const char file[], int line, string desc)
{
  throw test_failure(file, line, desc);
}


void check(
	const char file[],
	int line,
	bool condition,
	const char text[],
	string desc)
{
  if (!condition)
    throw test_failure(
	file,
	line,
	desc + " (failed expression: " + text + ")");
}


void expected_exception(const string &message)
{
  cout << "(Expected) " << message << endl;
}


string list_row(row Obj)
{
  return separated_list(", ", Obj.begin(), Obj.end(), deref_field);
}


string list_result(result Obj)
{
  if (Obj.empty()) return "<empty>";
  return "{" + separated_list("}\n{", Obj) + "}";
}


string list_result_iterator(result::const_iterator Obj)
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


int main(int, const char *argv[])
{
  const char *const test_name = argv[1];
  const test_map &tests = register_test(nullptr);

  int test_count = 0;
  list<string> failed;
  for (const auto &i: tests)
    if (!test_name || test_name == i.first)
    {
      cout << endl << "Running: " << i.first << endl;

      bool success = false;
      try
      {
        i.second->run();
        success = true;
      }
      catch (const test_failure &e)
      {
        cerr << "Test failure in " + e.file() + " line " +
	    to_string(e.line()) << ": " << e.what() << endl;
      }
      catch (const bad_alloc &)
      {
        cerr << "Out of memory!" << endl;
      }
      catch (const feature_not_supported &e)
      {
        cerr << "Not testing unsupported feature: " << e.what() << endl;
        success = true;
        --test_count;
      }
      catch (const sql_error &e)
      {
        cerr << "SQL error: " << e.what() << endl
             << "Query was: " << e.query() << endl;
      }
      catch (const exception &e)
      {
        cerr << "Exception: " << e.what() << endl;
      }
      catch (...)
      {
        cerr << "Unknown exception" << endl;
      }

      if (!success)
      {
        cerr << "FAILED: " << i.first << endl;
        failed.push_back(i.first);
      }
      ++test_count;
    }

  cout << "Ran " << test_count << " test(s)." << endl;

  if (!failed.empty())
  {
    cerr << "*** " << failed.size() << " test(s) failed: ***" << endl;
    for (const auto &i: failed) cerr << "\t" << i << endl;
  }

  return int(failed.size());
}
