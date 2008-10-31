#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_tablestream(connection_base &, transaction_base &Tsource)
{
  connection Cdest;
  work Tdest(Cdest);

  Tsource.exec("CREATE TEMP TABLE source(x integer, y varchar)");
  Tsource.exec("INSERT INTO source VALUES (0, 'zero')");
  Tsource.exec("INSERT INTO source VALUES (1, NULL)");
  Tsource.exec("INSERT INTO source VALUES (NULL, 'one')");
  Tsource.exec("INSERT INTO source VALUES (NULL, NULL)");

  Tdest.exec("CREATE TEMP TABLE dest(x integer, y varchar)");

  tablereader source(Tsource, "source");
  tablewriter dest(Tdest, "dest");

  result::size_type count = 0;
  for (string line; source.get_raw_line(line); dest.write_raw_line(line))
    ++count;

  source.complete();
  dest.complete();

  const result
	Rsource = Tsource.exec("SELECT x FROM source ORDER BY x"),
	Rdest = Tdest.exec("SELECT x FROM dest ORDER BY x");

  PQXX_CHECK_EQUAL(
	count,
	Rsource.size(),
	"Did not copy expected number of lines.");
  PQXX_CHECK_EQUAL(Rsource, Rdest, "Inconsistent raw-line tablestream copy.");
}
}

PQXX_REGISTER_TEST(test_tablestream)
