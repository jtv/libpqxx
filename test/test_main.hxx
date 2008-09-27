/* main() definition for libpqxx test runners.
 */

pqxx::test::base_test::~base_test() {}


const pqxx::test::test_map &pqxx::test::register_test(base_test *tc)
{
  static test_map tests;
  if (tc) tests[tc->name()] = tc;
  return tests;
}


int main(int, char *argv[])
{
  const char *const test_name = argv[1];
  const pqxx::test::test_map &tests = pqxx::test::register_test(NULL);

  int test_count = 0, failures = 0;
  for (pqxx::test::test_map::const_iterator i = tests.begin();
       i != tests.end();
       ++i)
    if (!test_name || test_name == i->first)
    {
      PGSTD::cout << PGSTD::endl << "Running: " << i->first << PGSTD::endl;
      const int outcome = i->second->run();
      if (outcome != 0) ++failures;
      ++test_count;
    }

  PGSTD::cout << "Ran " << test_count << " test(s)." << PGSTD::endl;

  if (failures > 0)
    PGSTD::cout << "*** " << failures << " test(s) failed. ***" << PGSTD::endl;

  return failures;
}
