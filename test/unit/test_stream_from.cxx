#include "../test_helpers.hxx"
#include "../test_types.hxx"

#include <pqxx/stream_from>

#include <cstring>
#include <iostream>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include <optional>


namespace
{
void test_nonoptionals(pqxx::connection_base &connection)
{
  pqxx::work tx{connection};
  pqxx::stream_from extractor{tx, "stream_from_test"};
  PQXX_CHECK(extractor, "stream_from failed to initialize");

  std::tuple<int, std::string, int, ipv4, std::string, bytea> got_tuple;

  extractor >> got_tuple;
  PQXX_CHECK(extractor, "stream_from failed to read first row");
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 1234, "field value mismatch");
  // PQXX_CHECK_EQUAL(*std::get<1>(got_tuple), , "field value mismatch");
  PQXX_CHECK_EQUAL(std::get<2>(got_tuple), 4321, "field value mismatch");
  PQXX_CHECK_EQUAL(
    std::get<3>(got_tuple), (ipv4{8, 8, 8, 8}), "field value mismatch");
  PQXX_CHECK_EQUAL(
    std::get<4>(got_tuple), "hello world", "field value mismatch");
  PQXX_CHECK_EQUAL(
    std::get<5>(got_tuple), (bytea{'\x00', '\x01', '\x02'}),
    "field value mismatch");

  try
  {
    extractor >> got_tuple;
    PQXX_CHECK_NOTREACHED("stream_from improperly read second row");
  }
  catch (pqxx::conversion_error const &e)
  {
    std::string what{e.what()},
      expected{"Attempt to convert null to " + pqxx::type_name<int> + "."};
    if (what != expected)
      throw;
    pqxx::test::expected_exception("Could not extract row: " + what);
  }

  std::tuple<
    int, std::string, std::nullptr_t, std::nullptr_t, std::string, bytea>
    got_tuple_nulls1;
  std::tuple<
    int, std::nullptr_t, std::nullptr_t, std::nullptr_t, std::string, bytea>
    got_tuple_nulls2;

  try
  {
    extractor >> got_tuple_nulls2;
    PQXX_CHECK_NOTREACHED("stream_from improperly read second row");
  }
  catch (pqxx::conversion_error const &e)
  {
    if (std::string{e.what(), 27} != "Attempt to convert non-null")
      throw;
    pqxx::test::expected_exception(
      std::string{"Could not extract row: "} + e.what());
  }

  extractor >> got_tuple_nulls1;
  PQXX_CHECK(extractor, "stream_from failed to reentrantly read second row");
  extractor >> got_tuple_nulls2;
  PQXX_CHECK(extractor, "stream_from failed to reentrantly read third row");
  extractor >> got_tuple;
  PQXX_CHECK(not extractor, "stream_from failed to detect end of stream");

  extractor.complete();

  PQXX_CHECK_SUCCEEDS(
    tx.exec1("SELECT 1"), "Could not use transaction after stream_from.");
}


void test_bad_tuples(pqxx::connection_base &conn)
{
  pqxx::work tx{conn};
  pqxx::stream_from extractor{tx, "stream_from_test"};
  PQXX_CHECK(extractor, "stream_from failed to initialize");

  std::tuple<int> got_tuple_too_short;
  try
  {
    extractor >> got_tuple_too_short;
    PQXX_CHECK_NOTREACHED("stream_from improperly read first row");
  }
  catch (pqxx::usage_error const &e)
  {
    std::string what{e.what()};
    if (what != "Not all fields extracted from stream_from line")
      throw;
    pqxx::test::expected_exception("Could not extract row: " + what);
  }

  std::tuple<int, std::string, int, ipv4, std::string, bytea, std::string>
    got_tuple_too_long;
  try
  {
    extractor >> got_tuple_too_long;
    PQXX_CHECK_NOTREACHED("stream_from improperly read first row");
  }
  catch (pqxx::usage_error const &e)
  {
    std::string what{e.what()};
    if (what != "Too few fields to extract from stream_from line.")
      throw;
    pqxx::test::expected_exception("Could not extract row: " + what);
  }

  extractor.complete();
}


#define ASSERT_FIELD_EQUAL(OPT, VAL)                                          \
  PQXX_CHECK(static_cast<bool>(OPT), "unexpected null field");                \
  PQXX_CHECK_EQUAL(*OPT, VAL, "field value mismatch")
#define ASSERT_FIELD_NULL(OPT)                                                \
  PQXX_CHECK(not static_cast<bool>(OPT), "expected null field")


template<template<typename...> class O>
void test_optional(pqxx::connection_base &connection)
{
  pqxx::work tx{connection};
  pqxx::stream_from extractor{tx, "stream_from_test"};
  PQXX_CHECK(extractor, "stream_from failed to initialize");

  std::tuple<int, O<std::string>, O<int>, O<ipv4>, O<std::string>, O<bytea>>
    got_tuple;

  extractor >> got_tuple;
  PQXX_CHECK(extractor, "stream_from failed to read first row.");
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 1234, "Field value mismatch.");
  PQXX_CHECK(
    static_cast<bool>(std::get<1>(got_tuple)), "Unexpected null field.");
  // PQXX_CHECK_EQUAL(*std::get<1>(got_tuple), , "field value mismatch");
  ASSERT_FIELD_EQUAL(std::get<2>(got_tuple), 4321);
  ASSERT_FIELD_EQUAL(std::get<3>(got_tuple), (ipv4{8, 8, 8, 8}));
  ASSERT_FIELD_EQUAL(std::get<4>(got_tuple), "hello world");
  ASSERT_FIELD_EQUAL(std::get<5>(got_tuple), (bytea{'\x00', '\x01', '\x02'}));

  extractor >> got_tuple;
  PQXX_CHECK(extractor, "stream_from failed to read second row");
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 5678, "field value mismatch");
  ASSERT_FIELD_EQUAL(std::get<1>(got_tuple), "2018-11-17 21:23:00");
  ASSERT_FIELD_NULL(std::get<2>(got_tuple));
  ASSERT_FIELD_NULL(std::get<3>(got_tuple));
  ASSERT_FIELD_EQUAL(std::get<4>(got_tuple), "こんにちは");
  ASSERT_FIELD_EQUAL(
    std::get<5>(got_tuple), (bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'}));

  extractor >> got_tuple;
  PQXX_CHECK(extractor, "stream_from failed to read third row");
  PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 910, "field value mismatch");
  ASSERT_FIELD_NULL(std::get<1>(got_tuple));
  ASSERT_FIELD_NULL(std::get<2>(got_tuple));
  ASSERT_FIELD_NULL(std::get<3>(got_tuple));
  ASSERT_FIELD_EQUAL(std::get<4>(got_tuple), "\\N");
  ASSERT_FIELD_EQUAL(std::get<5>(got_tuple), bytea{});

  extractor >> got_tuple;
  PQXX_CHECK(not extractor, "stream_from failed to detect end of stream");

  extractor.complete();
}


void test_stream_from()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  tx.exec0(
    "CREATE TEMP TABLE stream_from_test ("
    "number0 INT NOT NULL,"
    "ts1     TIMESTAMP NULL,"
    "number2 INT NULL,"
    "addr3   INET NULL,"
    "txt4    TEXT NULL,"
    "bin5    BYTEA NOT NULL"
    ")");
  tx.exec_params(
    "INSERT INTO stream_from_test VALUES ($1,$2,$3,$4,$5,$6)", 1234, "now",
    4321, ipv4{8, 8, 8, 8}, "hello world", bytea{'\x00', '\x01', '\x02'});
  tx.exec_params(
    "INSERT INTO stream_from_test VALUES ($1,$2,$3,$4,$5,$6)", 5678,
    "2018-11-17 21:23:00", nullptr, nullptr, "こんにちは",
    bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'});
  tx.exec_params(
    "INSERT INTO stream_from_test VALUES ($1,$2,$3,$4,$5,$6)", 910, nullptr,
    nullptr, nullptr, "\\N", bytea{});
  tx.commit();

  test_nonoptionals(conn);
  test_bad_tuples(conn);
  std::cout << "testing `std::unique_ptr` as optional...\n";
  test_optional<std::unique_ptr>(conn);
  std::cout << "testing `std::optional` as optional...\n";
  test_optional<std::optional>(conn);
}


void test_stream_from__escaping()
{
  std::string const input{"a\t\n\n\n \\b\nc"};
  pqxx::connection conn;
  pqxx::work tx{conn};
  tx.exec0("CREATE TEMP TABLE badstr (str text)");
  tx.exec0("INSERT INTO badstr (str) VALUES (" + tx.quote(input) + ")");
  pqxx::stream_from reader{tx, "badstr"};
  std::tuple<std::string> out;
  reader >> out;
  PQXX_CHECK_EQUAL(
    std::get<0>(out), input, "stream_from got weird characters wrong.");
}


void test_stream_from__iteration()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  tx.exec0("CREATE TEMP TABLE str (s text)");
  tx.exec0("INSERT INTO str (s) VALUES ('foo')");
  pqxx::stream_from reader{tx, "str"};

  int i{0};
  std::string out;
  for (std::tuple<std::string> t : reader.iter<std::string>())
  {
    i++;
    out = std::get<0>(t);
  }
  PQXX_CHECK_EQUAL(i, 1, "Wrong number of iterations.");
  PQXX_CHECK_EQUAL(out, "foo", "Got wrong string.");

  tx.exec0("INSERT INTO str (s) VALUES ('bar')");
  i = 0;
  std::set<std::string> strings;
  pqxx::stream_from reader2{tx, "str"};
  for (std::tuple<std::string> t : reader2.iter<std::string>())
  {
    i++;
    strings.insert(std::get<0>(t));
  }
  PQXX_CHECK_EQUAL(i, 2, "Wrong number of iterations.");
  PQXX_CHECK_EQUAL(strings.size(), 2u, "Wrong number of strings retrieved.");
  PQXX_CHECK(strings.find("foo") != strings.end(), "Missing key.");
  PQXX_CHECK(strings.find("bar") != strings.end(), "Missing key.");
}


void test_transaction_stream_from()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  tx.exec0("CREATE TEMP TABLE sample (id integer, name varchar)");
  tx.exec0("INSERT INTO sample (id, name) VALUES (321, 'something')");

  int items{0};
  int id{0};
  std::string name;

  for (auto item : tx.stream<int, std::string>("SELECT id, name FROM sample"))
  {
    items++;
    id = std::get<0>(item);
    name = std::get<1>(item);
  }
  PQXX_CHECK_EQUAL(items, 1, "Wrong number of iterations.");
  PQXX_CHECK_EQUAL(id, 321, "Got wrong int.");
  PQXX_CHECK_EQUAL(name, "something", "Got wrong string.");
}


PQXX_REGISTER_TEST(test_stream_from);
PQXX_REGISTER_TEST(test_stream_from__escaping);
PQXX_REGISTER_TEST(test_stream_from__iteration);
PQXX_REGISTER_TEST(test_transaction_stream_from);
} // namespace
