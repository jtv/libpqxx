#include <test_helpers.hxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void test_generate(transaction_base &t)
{
  t.exec("CREATE TEMP TABLE test_generate (x integer)");
  tablewriter writer(t, "test_generate");

  const char *single_field[] = {"Field"};
  PQXX_CHECK_EQUAL(
	"Field",
	writer.generate(single_field, single_field + 1),
	"Bad result from tablewriter::generate().");

  const char *multicolumn_row[] = {"One", "Two", "Three"};
  PQXX_CHECK_EQUAL(
	"One\tTwo\tThree",
	writer.generate(multicolumn_row, multicolumn_row + 3),
	"tablewriter::generate() breaks on multiple columns.");

  const char *row_with_null[] = {"One", NULL, "Three"};
  PQXX_CHECK_EQUAL(
	"One\t\\N\tThree",
	writer.generate(row_with_null, row_with_null + 3),
	"tablewriter::generate() breaks on NULL.");
}


void test_tablestream_scenario(transaction_base &Tsource)
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

void test_tablestream(transaction_base &t)
{
  test_generate(t);
  test_tablestream_scenario(t);
}
}

PQXX_REGISTER_TEST(test_tablestream)
