#include <pqxx/transaction>

#include "helpers.hxx"
#include "sample_types.hxx"

#include <cstring>
#include <string>
#include <vector>

#include <optional>


namespace
{
void test_stream_handles_empty(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  for (auto [out] : tx.stream<int>("SELECT generate_series(1, 0)"))
    PQXX_CHECK(false, "Unexpectedly got a value: " + pqxx::to_string(out));
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 99"), 99);
}


void test_stream_does_escaping(pqxx::test::context &)
{
  std::string const input{"a\t\n\n\n \\b\nc"};
  pqxx::connection cx;
  pqxx::work tx{cx};
  int counter{0};
  for (auto [out] : tx.stream<std::string_view>("SELECT " + tx.quote(input)))
  {
    PQXX_CHECK_EQUAL(out, input);
    ++counter;
  }
  PQXX_CHECK_EQUAL(counter, 1);
}


void test_stream_iterates(pqxx::test::context &)
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
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 99"), 99);
  tx.commit();

  PQXX_CHECK_EQUAL(std::size(ids), 2u);
  PQXX_CHECK_EQUAL(std::size(values), 2u);
  PQXX_CHECK_EQUAL(ids.at(0), 1);
  PQXX_CHECK_EQUAL(values.at(0), "String 1.");
  PQXX_CHECK_EQUAL(ids.at(1), 2);
  PQXX_CHECK_EQUAL(values.at(1), "String 2.");
}


void test_stream_reads_simple_values(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  int counter{0};
  for (auto [id, name] :
       tx.stream<std::size_t, std::string>("SELECT 213, 'Hi'"))
  {
    PQXX_CHECK_EQUAL(id, 213u);
    PQXX_CHECK_EQUAL(name, "Hi");
    ++counter;
  }
  PQXX_CHECK_EQUAL(counter, 1);
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 333"), 333);
}


void test_stream_reads_string_view(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  std::vector<std::string> out;
  for (auto [v] : tx.stream<std::string_view>(
         "SELECT 'x' || generate_series FROM generate_series(1, 2)"))
  {
    static_assert(std::is_same_v<decltype(v), std::string_view>);
    out.emplace_back(v);
  }
  PQXX_CHECK_EQUAL(std::size(out), 2u);
  PQXX_CHECK_EQUAL(out.at(0), "x1");
  PQXX_CHECK_EQUAL(out.at(1), "x2");
}


void test_stream_reads_nulls_as_optionals(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  for (auto [null] : tx.stream<std::optional<std::string>>("SELECT NULL"))
    PQXX_CHECK(not null.has_value(), "NULL translated to nonempty optional.");

  for (auto [val] : tx.stream<std::optional<std::string>>("SELECT 'x'"))
  {
    PQXX_CHECK(val.has_value());
    PQXX_CHECK_EQUAL(val.value_or("empty"), "x");
  }
}


void test_stream_reads_arrays(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  int count{0};
  for (auto [a] : tx.stream<pqxx::array<int>>("SELECT ARRAY[1,-42]"))
  {
    PQXX_CHECK_EQUAL(a[0], 1);
    PQXX_CHECK_EQUAL(a[1], -42);
    ++count;
  }
  PQXX_CHECK_EQUAL(count, 1);

  count = 0;
  cx.set_client_encoding("SJIS");

  // Some SJIS-encoded strings with multibyte characters that happen to contain
  // bytes with the same numeric values as the ASCII characters for backslash
  // and a closing brace.  If we were to parse this in the wrong encoding
  // group, things would go horribly wrong.
  auto const painful_array{"SELECT ARRAY['\x81\x5c', '\x81\x7d']"};
  for (auto [a] : tx.stream<pqxx::array<std::string>>(painful_array))
  {
    PQXX_CHECK_EQUAL(a.dimensions(), 1u);
    auto const sizes{a.sizes()};
    PQXX_CHECK_EQUAL(std::size(sizes), 1u);
    PQXX_CHECK_EQUAL(sizes[0], 2u);
    PQXX_CHECK_EQUAL(a[0], "\x81\x5c");
    PQXX_CHECK_EQUAL(a[1], "\x81\x7d");
    ++count;
  }
  PQXX_CHECK_EQUAL(count, 1);
}


void test_stream_parses_awkward_strings(pqxx::test::context &)
{
  pqxx::connection cx;

  bool const ascii_db{cx.get_var("server_encoding") == "SQL_ASCII"};

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
      "(4, '\t'), "
      "(5, '\\\\\\\n\\\\')")
    .no_rows();

  if (not ascii_db)
  {
    // An SJIS multibyte character that ends in a byte that happens to be the
    // ASCII value for a backslash.  This is one example of how an SJIS SQL
    // injection can break out of a string.
    tx.exec("INSERT INTO nasty(id, value) VALUES (6, '\x81\x5c')").no_rows();
  }

  std::vector<std::optional<std::string>> values;
  for (auto [id, value] : tx.stream<std::size_t, std::optional<std::string>>(
         "SELECT id, value FROM nasty ORDER BY id"))
  {
    PQXX_CHECK_EQUAL(id, std::size(values), "Test data is broken.");
    values.push_back(value);
  }

  PQXX_CHECK(not values.at(0).has_value(), "Null did not work properly.");
  PQXX_CHECK(values.at(1).has_value(), "String 'NULL' became a NULL.");
  PQXX_CHECK_EQUAL(
    values.at(1).value_or("empty"), "NULL", "String 'NULL' went badly.");
  PQXX_CHECK(values.at(2).has_value(), "String '\\N' became a NULL.");
  PQXX_CHECK_EQUAL(
    values.at(2).value_or("empty"), "\\N", "String '\\N' went badly.");
  PQXX_CHECK(values.at(3).has_value(), "String \"'NULL'\" became a NULL.");
  PQXX_CHECK_EQUAL(
    values.at(3).value_or("empty"), "'NULL'", "String \"'NULL'\" went badly.");
  PQXX_CHECK_EQUAL(
    values.at(4).value_or("empty"), "\t", "Tab unescaped wrong.");
  PQXX_CHECK_EQUAL(
    values.at(5).value_or("empty"), "\\\\\\\n\\\\",
    "Backslashes confused stream.");
  if (not ascii_db)
    PQXX_CHECK_EQUAL(
      values.at(6).value_or("empty"), "\x81\x5c",
      "Finicky SJIS character went badly.");
}


void test_stream_handles_nulls_in_all_places(pqxx::test::context &)
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


void test_stream_handles_empty_string(pqxx::test::context &)
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
PQXX_REGISTER_TEST(test_stream_reads_arrays);
PQXX_REGISTER_TEST(test_stream_parses_awkward_strings);
PQXX_REGISTER_TEST(test_stream_handles_nulls_in_all_places);
PQXX_REGISTER_TEST(test_stream_handles_empty_string);
} // namespace
