#include <pqxx/stream_from>
#include <pqxx/transaction>

#include "helpers.hxx"
#include "sample_types.hxx"

#include <cstring>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <optional>


namespace
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
void test_stream_from_nonoptionals(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto extractor{pqxx::stream_from::query(
    tx, "SELECT * FROM stream_from_test ORDER BY number0")};
  PQXX_CHECK(extractor);

  std::tuple<int, std::string, int, ipv4, std::string, bytea> got_tuple;

  try
  {
    // We can't read the "910" row -- it contains nulls, which our tuple does
    // not accept.
    extractor >> got_tuple;
    pqxx::test::check_notreached(
      "Failed to fail to stream null values into null-less fields.");
  }
  catch (pqxx::conversion_error const &e)
  {
    std::string const what{e.what()};
    if (not pqxx::str_contains(what, "null"))
      throw;
    pqxx::test::expected_exception(
      "Could not stream nulls into null-less fields: " + what);
  }

  // The stream is still good though.
  // The second tuple is fine.
  extractor >> got_tuple;
  PQXX_CHECK(extractor);

  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 1234);
  // Don't know much about the timestamp, but let's assume it starts with a
  // year in the second millennium.
  PQXX_CHECK(std::get<1>(got_tuple).at(0) == '2');
  PQXX_CHECK_LESS(std::size(std::get<1>(got_tuple)), 40u);
  PQXX_CHECK_GREATER(std::size(std::get<1>(got_tuple)), 20u);
  PQXX_CHECK_EQUAL(std::get<2>(got_tuple), 4321);
  PQXX_CHECK_EQUAL(std::get<3>(got_tuple), (ipv4{8, 8, 8, 8}));
  PQXX_CHECK_EQUAL(std::get<4>(got_tuple), "hello\n \tworld");
  PQXX_CHECK_EQUAL(std::get<5>(got_tuple), (bytea{'\x00', '\x01', '\x02'}));

  // The third tuple contains some nulls. For what it's worth, when we *know*
  // that we're getting nulls, we can stream them into nullptr_t fields.
  std::tuple<
    int, std::string, std::nullptr_t, std::nullptr_t, std::string, bytea>
    tup_w_nulls;

  extractor >> tup_w_nulls;
  PQXX_CHECK(extractor, "Stream ended prematurely.");

  PQXX_CHECK_EQUAL(std::get<0>(tup_w_nulls), 5678);
  PQXX_CHECK(std::get<2>(tup_w_nulls) == nullptr);
  PQXX_CHECK(std::get<3>(tup_w_nulls) == nullptr);

  // We're at the end of the stream.
  extractor >> tup_w_nulls;
  PQXX_CHECK(not extractor, "Stream did not end.");

  // Of course we can't stream a non-null value into a nullptr field.
  auto ex2{pqxx::stream_from::query(tx, "SELECT 1")};
  std::tuple<std::nullptr_t> null_tup;
  try
  {
    ex2 >> null_tup;
    pqxx::test::check_notreached(
      "stream_from should have refused to convert non-null value to "
      "nullptr_t.");
  }
  catch (pqxx::conversion_error const &e)
  {
    std::string const what{e.what()};
    if (not pqxx::str_contains(what, "null"))
      throw;
    pqxx::test::expected_exception(
      std::string{"Could not extract row: "} + what);
  }
  ex2 >> null_tup;
  PQXX_CHECK(not ex2, "Stream did not end.");

  PQXX_CHECK_SUCCEEDS(tx.exec1("SELECT 1"));
}


void test_bad_tuples(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  auto extractor{pqxx::stream_from::table(tx, {"stream_from_test"})};
  PQXX_CHECK(extractor);

  std::tuple<int> got_tuple_too_short;
  try
  {
    extractor >> got_tuple_too_short;
    pqxx::test::check_notreached("stream_from improperly read first row");
  }
  catch (pqxx::usage_error const &e)
  {
    std::string const what{e.what()};
    if (not pqxx::str_contains(what, '1') or not pqxx::str_contains(what, '6'))
      throw;
    pqxx::test::expected_exception("Tuple is wrong size: " + what);
  }

  std::tuple<int, std::string, int, ipv4, std::string, bytea, std::string>
    got_tuple_too_long;
  try
  {
    extractor >> got_tuple_too_long;
    pqxx::test::check_notreached("stream_from improperly read first row");
  }
  catch (pqxx::usage_error const &e)
  {
    std::string const what{e.what()};
    if (not pqxx::str_contains(what, '6') or not pqxx::str_contains(what, '7'))
      throw;
    pqxx::test::expected_exception("Could not extract row: " + what);
  }

  extractor.complete();
}


// Annoying: clang-tidy complains about an unchecked std::optional access,
// even if we check that OPT contains a value before we retrieve it.
#define ASSERT_FIELD_EQUAL(OPT, VAL)                                          \
  PQXX_CHECK(static_cast<bool>(OPT), "unexpected null field");                \
  if ((OPT))                                                                  \
  PQXX_CHECK_EQUAL(*(OPT), (VAL), "field value mismatch")

#define ASSERT_FIELD_NULL(OPT)                                                \
  PQXX_CHECK(not static_cast<bool>(OPT), "expected null field")


/// Japanese text: \u3053\u3093\u306b\u3061\u308f ("konichiwa," a greeting).
constexpr std::string_view japanese_utf8{
  "\xe3\x81\x93\xe3\x82\x93\xe3\x81\xab\xe3\x81\xa1\xe3\x82\x8f"};


template<template<typename...> class O>
void test_stream_from_optional(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto extractor{pqxx::stream_from::query(
    tx, "SELECT * FROM stream_from_test ORDER BY number0")};
  PQXX_CHECK(extractor);

  std::tuple<int, O<std::string>, O<int>, O<ipv4>, O<std::string>, O<bytea>>
    got_tuple;

  extractor >> got_tuple;
  PQXX_CHECK(extractor);
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 910);
  ASSERT_FIELD_NULL(std::get<1>(got_tuple));
  ASSERT_FIELD_NULL(std::get<2>(got_tuple));
  ASSERT_FIELD_NULL(std::get<3>(got_tuple));

  // These look like false positives from clang-tidy: ASSERT_FIELD_EQUAL
  // does check for empty optionals, yet we get a warning about a check being
  // needed.

  // NOLINTBEGIN(bugprone-unchecked-optional-access)
  ASSERT_FIELD_EQUAL((std::get<4>(got_tuple)), "\\N");
  ASSERT_FIELD_EQUAL((std::get<5>(got_tuple)), bytea{});

  extractor >> got_tuple;
  PQXX_CHECK(extractor);
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 1234);
  PQXX_CHECK(static_cast<bool>(std::get<1>(got_tuple)));
  ASSERT_FIELD_EQUAL((std::get<2>(got_tuple)), 4321);
  ASSERT_FIELD_EQUAL((std::get<3>(got_tuple)), (ipv4{8, 8, 8, 8}));
  ASSERT_FIELD_EQUAL((std::get<4>(got_tuple)), "hello\n \tworld");
  ASSERT_FIELD_EQUAL(
    (std::get<5>(got_tuple)), (bytea{'\x00', '\x01', '\x02'}));

  extractor >> got_tuple;
  PQXX_CHECK(extractor);
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 5678);
  ASSERT_FIELD_EQUAL((std::get<1>(got_tuple)), "2018-11-17 21:23:00");
  ASSERT_FIELD_NULL(std::get<2>(got_tuple));
  ASSERT_FIELD_NULL(std::get<3>(got_tuple));
  ASSERT_FIELD_EQUAL((std::get<4>(got_tuple)), japanese_utf8);
  ASSERT_FIELD_EQUAL(
    (std::get<5>(got_tuple)),
    (bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'}));
  // NOLINTEND(bugprone-unchecked-optional-access)

  extractor >> got_tuple;
  PQXX_CHECK(not extractor, "stream_from failed to detect end of stream");

  extractor.complete();
}


void test_stream_from(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec0(
    "CREATE TEMP TABLE stream_from_test ("
    "number0 INT NOT NULL,"
    "ts1     TIMESTAMP NULL,"
    "number2 INT NULL,"
    "addr3   INET NULL,"
    "txt4    TEXT NULL,"
    "bin5    BYTEA NOT NULL"
    ")");
  tx.exec(
    "INSERT INTO stream_from_test VALUES ($1,$2,$3,$4,$5,$6)",
    pqxx::params{tx, 910, nullptr, nullptr, nullptr, "\\N", bytea{}});
  tx.exec(
    "INSERT INTO stream_from_test VALUES ($1,$2,$3,$4,$5,$6)",
    pqxx::params{
      tx, 1234, "now", 4321, ipv4{8, 8, 8, 8}, "hello\n \tworld",
      bytea{'\x00', '\x01', '\x02'}});
  tx.exec(
    "INSERT INTO stream_from_test VALUES ($1,$2,$3,$4,$5,$6)",
    pqxx::params{
      tx, 5678, "2018-11-17 21:23:00", nullptr, nullptr, japanese_utf8,
      bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'}});
  tx.commit();

  test_stream_from_nonoptionals(cx);
  test_bad_tuples(cx);
  test_stream_from_optional<std::unique_ptr>(cx);
  test_stream_from_optional<std::optional>(cx);
}


void test_stream_from_does_escaping(pqxx::test::context &)
{
  std::string const input{"a\t\n\n\n \\b\nc"};
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE badstr (str text)").no_rows();
  tx.exec("INSERT INTO badstr (str) VALUES ($1)", pqxx::params{tx, input})
    .no_rows();
  auto reader{pqxx::stream_from::table(tx, {"badstr"})};
  std::tuple<std::string> out;
  reader >> out;
  PQXX_CHECK_EQUAL(std::get<0>(out), input);
}


void test_stream_from_does_iteration(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec0("CREATE TEMP TABLE str (s text)");
  tx.exec0("INSERT INTO str (s) VALUES ('foo')");
  auto reader{pqxx::stream_from::table(tx, {"str"})};

  int i{0};
  std::string out;
  for (std::tuple<std::string> const &t : reader.iter<std::string>())
  {
    i++;
    out = std::get<0>(t);
  }
  PQXX_CHECK_EQUAL(i, 1);
  PQXX_CHECK_EQUAL(out, "foo");

  tx.exec0("INSERT INTO str (s) VALUES ('bar')");
  i = 0;
  std::set<std::string> strings;
  auto reader2{pqxx::stream_from::table(tx, {"str"})};
  for (std::tuple<std::string> const &t : reader2.iter<std::string>())
  {
    i++;
    strings.insert(std::get<0>(t));
  }
  PQXX_CHECK_EQUAL(i, 2);
  PQXX_CHECK_EQUAL(std::size(strings), 2u);
  PQXX_CHECK(strings.contains("foo"));
  PQXX_CHECK(strings.contains("bar"));
}


void test_stream_from_read_row(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec0("CREATE TEMP TABLE sample (id integer, name varchar, opt integer)");
  tx.exec0("INSERT INTO sample (id, name) VALUES (321, 'something')");

  auto stream{pqxx::stream_from::table(tx, {"sample"})};
  auto fields{stream.read_row()};
  PQXX_CHECK(fields != nullptr);
  // (Work around a weird false positive in clang-tidy where the check that
  // fields is _not_ null somehow convinces the checker that it _is_ null.)
  assert(fields != nullptr);
  PQXX_CHECK_EQUAL(fields->size(), 3ul);
  PQXX_CHECK_EQUAL(std::string((*fields)[0]), "321");
  PQXX_CHECK_EQUAL(std::string((*fields)[1]), "something");
  PQXX_CHECK(std::data((*fields)[2]) == nullptr);

  auto last{stream.read_row()};
  PQXX_CHECK(last == nullptr, "No null pointer at end of stream.");
}


void test_stream_from_parses_awkward_strings(pqxx::test::context &)
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
      "(3, '''NULL''')")
    .no_rows();

  if (not ascii_db)
  {
    // An SJIS multibyte character that ends in a byte that happens to be the
    // ASCII value for a backslash.  This is one example of how an SJIS SQL
    // injection can break out of a string.
    tx.exec("INSERT INTO nasty(id, value) VALUES (4, '\x81\x5c')").no_rows();
  }

  std::vector<std::optional<std::string>> values;
  for (auto const &[id, value] :
       tx.query<std::size_t, std::optional<std::string>>(
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

  if (not ascii_db)
    PQXX_CHECK_EQUAL(
      values.at(4).value_or("empty"), "\x81\x5c",
      "Finicky SJIS character went badly.");
}
#include "pqxx/internal/ignore-deprecated-post.hxx"


PQXX_REGISTER_TEST(test_stream_from);
PQXX_REGISTER_TEST(test_stream_from_does_escaping);
PQXX_REGISTER_TEST(test_stream_from_does_iteration);
PQXX_REGISTER_TEST(test_stream_from_read_row);
PQXX_REGISTER_TEST(test_stream_from_parses_awkward_strings);
} // namespace
