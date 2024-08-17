#include <pqxx/transaction>

#include "../test_helpers.hxx"
#include "../test_types.hxx"

#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <optional>


namespace
{
void test_stream_handles_empty()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  for (auto [out] : tx.stream<int>("SELECT generate_series(1, 0)"))
    PQXX_CHECK(false, "Unexpectedly got a value: " + pqxx::to_string(out));
  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 99"), 99,
    "Things went wrong after empty stream.");
}


void test_stream_does_escaping()
{
  std::string const input{"a\t\n\n\n \\b\nc"};
  pqxx::connection cx;
  pqxx::work tx{cx};
  int counter{0};
  for (auto [out] : tx.stream<std::string_view>("SELECT " + tx.quote(input)))
  {
    PQXX_CHECK_EQUAL(out, input, "stream got weird characters wrong.");
    ++counter;
  }
  PQXX_CHECK_EQUAL(counter, 1, "Expected exactly 1 iteration.");
}


void test_stream_iterates()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::vector<int> ids;
  std::vector<std::string> values;

  for (auto [id, value] : tx.stream<int, std::string>(
         "SELECT generate_series, 'String ' || generate_series::text || '.' "
         "FROM generate_series(1, 2)"))
  {
    ids.push_back(id);
    values.push_back(value);
  }
  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 99"), 99, "Things went wrong after stream.");
  tx.commit();

  PQXX_CHECK_EQUAL(std::size(ids), 2u, "Wrong number of rows.");
  PQXX_CHECK_EQUAL(std::size(values), 2u, "Wrong number of values.");
  PQXX_CHECK_EQUAL(ids[0], 1, "Wrong IDs.");
  PQXX_CHECK_EQUAL(values[0], "String 1.", "Wrong values.");
  PQXX_CHECK_EQUAL(ids[1], 2, "Wrong second ID.");
  PQXX_CHECK_EQUAL(values[1], "String 2.", "Wrong second value.");
}


void test_stream_reads_simple_values()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  int counter{0};
  for (auto [id, name] :
       tx.stream<std::size_t, std::string>("SELECT 213, 'Hi'"))
  {
    PQXX_CHECK_EQUAL(id, 213u, "Bad ID.");
    PQXX_CHECK_EQUAL(name, "Hi", "Bad name.");
    ++counter;
  }
  PQXX_CHECK_EQUAL(counter, 1, "Expected exactly 1 row.");
  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 333"), 333, "Bad value after stream.");
}


void test_stream_reads_string_view()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  std::vector<std::string> out;
  for (auto [v] : tx.stream<std::string_view>(
         "SELECT 'x' || generate_series FROM generate_series(1, 2)"))
  {
    static_assert(std::is_same_v<decltype(v), std::string_view>);
    out.push_back(std::string{v});
  }
  PQXX_CHECK_EQUAL(std::size(out), 2u, "Wrong number of rows.");
  PQXX_CHECK_EQUAL(out[0], "x1", "Wrong first value.");
  PQXX_CHECK_EQUAL(out[1], "x2", "Wrong second value.");
}


void test_stream_reads_nulls_as_optionals()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  for (auto [null] : tx.stream<std::optional<std::string>>("SELECT NULL"))
    PQXX_CHECK(not null.has_value(), "NULL translated to nonempty optional.");

  for (auto [val] : tx.stream<std::optional<std::string>>("SELECT 'x'"))
  {
    PQXX_CHECK(
      val.has_value(), "Non-null value did not come through as optional.");
    PQXX_CHECK_EQUAL(*val, "x", "Bad value in optional.");
  }
}


void test_stream_parses_awkward_strings()
{
  pqxx::connection cx;

  // This is a particularly awkward encoding that we should test.  Its
  // multibyte characters can include byte values that *look* like ASCII
  // characters, such as quotes and backslashes.  It is crucial that we parse
  // those properly.  A byte-for-byte scan could find special ASCII characters
  // that aren't really there.
  cx.set_client_encoding("SJIS");
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE nasty(id integer, value varchar)").no_rows();
  tx.exec(
      "INSERT INTO nasty(id, value) VALUES "
      // A proper null.
      "(0, NULL), "
      // Some strings that could easily be mis-parsed as null.
      "(1, 'NULL'), "
      "(2, '\\N'), "
      "(3, '''NULL'''), "
      // An SJIS multibyte character that ends in a byte that happens to be the
      // ASCII value for a backslash.  This is one example of how an SJIS SQL
      // injection can break out of a string.
      "(4, '\x81\x5c'), "
      "(5, '\t'), "
      "(6, '\\\\\\\n\\\\')")
    .no_rows();

  std::vector<std::optional<std::string>> values;
  for (auto [id, value] : tx.stream<std::size_t, std::optional<std::string>>(
         "SELECT id, value FROM nasty ORDER BY id"))
  {
    PQXX_CHECK_EQUAL(id, std::size(values), "Test data is broken.");
    values.push_back(value);
  }

  PQXX_CHECK(not values[0].has_value(), "Null did not work properly.");
  PQXX_CHECK(values[1].has_value(), "String 'NULL' became a NULL.");
  PQXX_CHECK_EQUAL(values[1].value(), "NULL", "String 'NULL' went badly.");
  PQXX_CHECK(values[2].has_value(), "String '\\N' became a NULL.");
  PQXX_CHECK_EQUAL(values[2].value(), "\\N", "String '\\N' went badly.");
  PQXX_CHECK(values[3].has_value(), "String \"'NULL'\" became a NULL.");
  PQXX_CHECK_EQUAL(
    values[3].value(), "'NULL'", "String \"'NULL'\" went badly.");
  PQXX_CHECK_EQUAL(
    values[4].value(), "\x81\x5c", "Finicky SJIS character went badly.");
  PQXX_CHECK_EQUAL(values[5].value(), "\t", "Tab unescaped wrong.");
  PQXX_CHECK_EQUAL(
    values[6].value(), "\\\\\\\n\\\\", "Backslashes confused stream.");
}


void test_stream_handles_nulls_in_all_places()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  for (auto [a, b, c, d, e] :
       tx.stream<
         std::optional<std::string>, std::optional<int>, int,
         std::optional<std::string>, std::optional<std::string>>(
         "SELECT NULL::text, NULL::integer, 11, NULL::text, NULL::text"))
  {
    PQXX_CHECK(not a, "Starting null did not come through.");
    PQXX_CHECK(not b, "Null in 2nd column did not come through.");
    PQXX_CHECK_EQUAL(c, 11, "Integer in the middle went wrong.");
    PQXX_CHECK(not d, "Null further in did not come through.");
    PQXX_CHECK(not e, "Final null did not come through.");
  }
}


void test_stream_handles_empty_string()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::string out{"<uninitialised>"};
  for (auto [empty] : tx.stream<std::string_view>("SELECT ''")) out = empty;
  PQXX_CHECK_EQUAL(out, "", "Empty string_view parsed wrong.");

  out = "<uniniutialised>";
  int num{0};
  for (auto [i, s] : tx.stream<int, std::string_view>("SELECT 99, ''"))
  {
    num = i;
    out = s;
  }
  PQXX_CHECK_EQUAL(num, 99, "Integer came out wrong before empty string.");
  PQXX_CHECK_EQUAL(out, "", "Final empty string came out wrong.");

  for (auto [s2, i2] : tx.stream<std::string_view, int>("SELECT '', 33"))
  {
    out = s2;
    num = i2;
  }
  PQXX_CHECK_EQUAL(out, "", "Leading empty string came out wrong.");
  PQXX_CHECK_EQUAL(num, 33, "Integer came out wrong after empty string.");
}


PQXX_REGISTER_TEST(test_stream_handles_empty);
PQXX_REGISTER_TEST(test_stream_does_escaping);
PQXX_REGISTER_TEST(test_stream_reads_simple_values);
PQXX_REGISTER_TEST(test_stream_reads_string_view);
PQXX_REGISTER_TEST(test_stream_iterates);
PQXX_REGISTER_TEST(test_stream_reads_nulls_as_optionals);
PQXX_REGISTER_TEST(test_stream_parses_awkward_strings);
PQXX_REGISTER_TEST(test_stream_handles_nulls_in_all_places);
PQXX_REGISTER_TEST(test_stream_handles_empty_string);
} // namespace
