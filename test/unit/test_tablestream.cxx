#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_tablestream(connection_base &, transaction_base &Tsource)
{
  connection Cdest;
  work Tdest(Cdest);

  // Copy straight from one table into another using a tablereader and a
  // tablewriter, on separate connections.
  Tsource.exec("CREATE TEMP TABLE source(x integer, y varchar)");
  Tsource.exec("INSERT INTO source VALUES (0, 'zero')");
  Tsource.exec("INSERT INTO source VALUES (1, NULL)");
  Tsource.exec("INSERT INTO source VALUES (NULL, 'one')");
  Tsource.exec("INSERT INTO source VALUES (NULL, NULL)");

  Tdest.exec("CREATE TEMP TABLE dest(x integer, y varchar)");

  result Rsource, Rdest;

  {
    tablereader source(Tsource, "source");
    tablewriter dest(Tdest, "dest");

    result::size_type count = 0;
    for (string line; source.get_raw_line(line); dest.write_raw_line(line))
      ++count;

    source.complete();
    dest.complete();

    Rsource = Tsource.exec("SELECT x FROM source ORDER BY x"),
    Rdest = Tdest.exec("SELECT x FROM dest ORDER BY x");

    PQXX_CHECK_EQUAL(
	count,
	Rsource.size(),
	"Did not copy expected number of lines.");
    PQXX_CHECK_EQUAL(Rsource, Rdest, "Inconsistent raw-line tablestream copy.");
  }

  // Now do the same, but with explicit column lists and an alternate
  // representation of NULL.  We could specify the columns in exactly the same
  // way on both, but let's keep things interesting.
  Tdest.exec("DELETE FROM dest");

  const char *colnames_array[] = { "x", "y" };

  vector<string> colnames_vector;
  colnames_vector.resize(2);
  colnames_vector[0] = "x";
  colnames_vector[1] = "y";

  tablereader source2(
	Tsource,
	"source",
	&colnames_array[0],
	&colnames_array[2],
	"nn");
  tablewriter dest2(
	Tdest,
	"dest",
	colnames_vector.begin(),
	colnames_vector.end(),
	string("nn"));

  dest2 << source2;
  source2.complete();
  dest2.complete();

  Rdest = Tdest.exec("SELECT x FROM dest ORDER BY x");
  PQXX_CHECK_EQUAL(Rsource, Rdest, "Customized tablestream copy went wrong.");
}
}

PQXX_REGISTER_TEST(test_tablestream)
