using namespace pqxx;
using namespace pqxx::test;

namespace
{
void run_tests(const char test_name[])
{
  const test_map &tests = register_test(NULL);
  int test_count = 0;
  for (test_map::const_iterator i = tests.begin(); i != tests.end(); ++i)
    if (!test_name || test_name == i->first)
    {
      PGSTD::cout << PGSTD::endl << "Running: " << i->first << PGSTD::endl;
      i->second->run();
      ++test_count;
    }
  PGSTD::cout << "Ran " << test_count << " test(s)." << PGSTD::endl;
}
}


namespace pqxx
{
namespace test
{
base_test::~base_test() {}

const test_map &register_test(base_test *tc)
{
  static test_map tests;
  if (tc) tests[tc->name()] = tc;
  return tests;
}
}
}


int main(int, char *argv[])
{
  run_tests(argv[1]);
}

