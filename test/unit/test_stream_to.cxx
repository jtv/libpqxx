#include <iostream>
#include <optional>

#include <pqxx/stream_to>
#include <pqxx/transaction>

#include "../test_helpers.hxx"
#include "../test_types.hxx"

namespace
{
using namespace std::literals;


std::string truncate_sql_error(std::string const &what)
{
  auto trunc{what.substr(0, what.find('\n'))};
  if (std::size(trunc) > 64)
    trunc = trunc.substr(0, 61) + "...";
  return trunc;
}


void test_nonoptionals(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  auto const nonascii{"\u3053\u3093\u306b\u3061\u308f"};
  bytea const binary{'\x00', '\x01', '\x02'},
    text{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'};

  inserter << std::make_tuple(
    1234, "now", 4321, ipv4{8, 8, 4, 4}, "hello nonoptional world", binary);
  inserter << std::make_tuple(
    5678, "2018-11-17 21:23:00", nullptr, nullptr, nonascii, text);
  inserter << std::make_tuple(910, nullptr, nullptr, nullptr, "\\N", bytea{});

  inserter.complete();

  auto r1{
    tx.exec("SELECT * FROM stream_to_test WHERE number0 = 1234").one_row()};
  PQXX_CHECK_EQUAL(r1[0].as<int>(), 1234, "Read back wrong first int.");
  PQXX_CHECK_EQUAL(
    r1[4].as<std::string>(), "hello nonoptional world",
    "Read back wrong string.");
  PQXX_CHECK_EQUAL(r1[3].as<ipv4>(), ipv4(8, 8, 4, 4), "Read back wrong ip.");
  PQXX_CHECK_EQUAL(r1[5].as<bytea>(), binary, "Read back wrong bytea.");

  auto r2{
    tx.exec("SELECT * FROM stream_to_test WHERE number0 = 5678").one_row()};
  PQXX_CHECK_EQUAL(r2[0].as<int>(), 5678, "Wrong int on second row.");
  PQXX_CHECK(r2[2].is_null(), "Field 2 was meant to be null.");
  PQXX_CHECK(r2[3].is_null(), "Field 3 was meant to be null.");
  PQXX_CHECK_EQUAL(r2[4].as<std::string>(), nonascii, "Wrong non-ascii text.");
  tx.commit();
}


void test_nonoptionals_fold(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  auto const nonascii{"\u3053\u3093\u306b\u3061\u308f"};
  bytea const binary{'\x00', '\x01', '\x02'},
    text{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'};

  inserter.write_values(
    1234, "now", 4321, ipv4{8, 8, 4, 4}, "hello nonoptional world", binary);
  inserter.write_values(
    5678, "2018-11-17 21:23:00", nullptr, nullptr, nonascii, text);
  inserter.write_values(910, nullptr, nullptr, nullptr, "\\N", bytea{});

  inserter.complete();

  auto r1{
    tx.exec("SELECT * FROM stream_to_test WHERE number0 = 1234").one_row()};
  PQXX_CHECK_EQUAL(r1[0].as<int>(), 1234, "Read back wrong first int.");
  PQXX_CHECK_EQUAL(
    r1[4].as<std::string>(), "hello nonoptional world",
    "Read back wrong string.");
  PQXX_CHECK_EQUAL(r1[3].as<ipv4>(), ipv4(8, 8, 4, 4), "Read back wrong ip.");
  PQXX_CHECK_EQUAL(r1[5].as<bytea>(), binary, "Read back wrong bytera.");

  auto r2{
    tx.exec("SELECT * FROM stream_to_test WHERE number0 = 5678").one_row()};
  PQXX_CHECK_EQUAL(r2[0].as<int>(), 5678, "Wrong int on second row.");
  PQXX_CHECK(r2[2].is_null(), "Field 2 was meant to be null.");
  PQXX_CHECK(r2[3].is_null(), "Field 3 was meant to be null.");
  PQXX_CHECK_EQUAL(r2[4].as<std::string>(), nonascii, "Wrong non-ascii text.");
  tx.commit();
}


/// Try to violate stream_to_test's not-null constraint using a stream_to.
void insert_bad_null_tuple(pqxx::stream_to &inserter)
{
  inserter << std::make_tuple(
    nullptr, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
    bytea{'\x00', '\x01', '\x02'});
  inserter.complete();
}


void test_bad_null(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");
  PQXX_CHECK_THROWS(
    insert_bad_null_tuple(inserter), pqxx::not_null_violation,
    "Did not expected not_null_violation when stream_to inserts a bad null.");
}


/// Try to violate stream_to_test's not-null construct using a stream_to.
void insert_bad_null_write(pqxx::stream_to &inserter)
{
  inserter.write_values(
    nullptr, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
    bytea{'\x00', '\x01', '\x02'});
  inserter.complete();
}


void test_bad_null_fold(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");
  PQXX_CHECK_THROWS(
    insert_bad_null_write(inserter), pqxx::not_null_violation,
    "Did not expected not_null_violation when stream_to inserts a bad null.");
}


void test_too_few_fields(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter << std::make_tuple(1234, "now", 4321, ipv4{8, 8, 8, 8});
    inserter.complete();
    tx.commit();
    PQXX_CHECK_NOTREACHED("stream_from improperly inserted row");
  }
  catch (pqxx::sql_error const &e)
  {
    std::string what{e.what()};
    if (what.find("missing data for column") == std::string::npos)
      throw;
    pqxx::test::expected_exception(
      "Could not insert row: " + truncate_sql_error(what));
  }
}

void test_too_few_fields_fold(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter.write_values(1234, "now", 4321, ipv4{8, 8, 8, 8});
    inserter.complete();
    tx.commit();
    PQXX_CHECK_NOTREACHED("stream_from_fold improperly inserted row");
  }
  catch (pqxx::sql_error const &e)
  {
    std::string what{e.what()};
    if (what.find("missing data for column") == std::string::npos)
      throw;
    pqxx::test::expected_exception(
      "Fold - Could not insert row: " + truncate_sql_error(what));
  }
}


void test_too_many_fields(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter << std::make_tuple(
      1234, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
      bytea{'\x00', '\x01', '\x02'}, 5678);
    inserter.complete();
    tx.commit();
    PQXX_CHECK_NOTREACHED("stream_from improperly inserted row");
  }
  catch (pqxx::sql_error const &e)
  {
    std::string what{e.what()};
    if (what.find("extra data") == std::string::npos)
      throw;
    pqxx::test::expected_exception(
      "Could not insert row: " + truncate_sql_error(what));
  }
}

void test_too_many_fields_fold(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter.write_values(
      1234, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
      bytea{'\x00', '\x01', '\x02'}, 5678);
    inserter.complete();
    tx.commit();
    PQXX_CHECK_NOTREACHED("stream_from_fold improperly inserted row");
  }
  catch (pqxx::sql_error const &e)
  {
    std::string what{e.what()};
    if (what.find("extra data") == std::string::npos)
      throw;
    pqxx::test::expected_exception(
      "Fold - Could not insert row: " + truncate_sql_error(what));
  }
}


void test_stream_to_does_nonnull_optional()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE foo(x integer, y text)").no_rows();
  auto inserter{pqxx::stream_to::table(tx, {"foo"})};
  inserter.write_values(
    std::optional<int>{368}, std::optional<std::string>{"Text"});
  inserter.complete();
  auto const row{tx.exec("SELECT x, y FROM foo").one_row()};
  PQXX_CHECK_EQUAL(
    row[0].as<std::string>(), "368", "Non-null int optional came out wrong.");
  PQXX_CHECK_EQUAL(
    row[1].as<std::string>(), "Text",
    "Non-null string optional came out wrong.");
}


template<template<typename...> class O>
void test_optional(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  inserter << std::make_tuple(
    910, O<std::string>{pqxx::nullness<O<std::string>>::null()},
    O<int>{pqxx::nullness<O<int>>::null()},
    O<ipv4>{pqxx::nullness<O<ipv4>>::null()}, "\\N", bytea{});

  inserter.complete();
  tx.commit();
}

template<template<typename...> class O>
void test_optional_fold(pqxx::connection &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  inserter.write_values(
    910, O<std::string>{pqxx::nullness<O<std::string>>::null()},
    O<int>{pqxx::nullness<O<int>>::null()},
    O<ipv4>{pqxx::nullness<O<ipv4>>::null()}, "\\N", bytea{});

  inserter.complete();
  tx.commit();
}


// As an alternative to a tuple, you can also insert a container.
void test_container_stream_to()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE test_container(a integer, b integer)").no_rows();

  auto inserter{pqxx::stream_to::table(tx, {"test_container"})};

  inserter << std::vector{112, 244};
  inserter.complete();

  auto read{tx.exec("SELECT * FROM test_container").one_row()};
  PQXX_CHECK_EQUAL(
    read[0].as<int>(), 112, "stream_to on container went wrong.");
  PQXX_CHECK_EQUAL(
    read[1].as<int>(), 244, "Second container field went wrong.");
  tx.commit();
}

void test_variant_fold(pqxx::connection_base &connection)
{
  pqxx::work tx{connection};
  auto inserter{pqxx::stream_to::table(tx, {"stream_to_test"})};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  inserter.write_values(
    std::variant<std::string, int>{1234},
    std::variant<float, std::string>{"now"}, 4321, ipv4{8, 8, 8, 8},
    "hello world", bytea{'\x00', '\x01', '\x02'});
  inserter.write_values(
    5678, "2018-11-17 21:23:00", nullptr, nullptr,
    "\u3053\u3093\u306b\u3061\u308f",
    bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'});
  inserter.write_values(910, nullptr, nullptr, nullptr, "\\N", bytea{});

  inserter.complete();
  tx.commit();
}

void clear_table(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  tx.exec("DELETE FROM stream_to_test").no_rows();
  tx.commit();
}


void test_stream_to()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec(
      "CREATE TEMP TABLE stream_to_test ("
      "number0 INT NOT NULL,"
      "ts1     TIMESTAMP NULL,"
      "number2 INT NULL,"
      "addr3   INET NULL,"
      "txt4    TEXT NULL,"
      "bin5    BYTEA NOT NULL"
      ")")
    .no_rows();
  tx.commit();

  test_nonoptionals(cx);
  clear_table(cx);
  test_nonoptionals_fold(cx);
  clear_table(cx);
  test_bad_null(cx);
  clear_table(cx);
  test_bad_null_fold(cx);
  clear_table(cx);
  test_too_few_fields(cx);
  clear_table(cx);
  test_too_few_fields_fold(cx);
  clear_table(cx);
  test_too_many_fields(cx);
  clear_table(cx);
  test_too_many_fields_fold(cx);
  clear_table(cx);
  test_optional<std::unique_ptr>(cx);
  clear_table(cx);
  test_optional_fold<std::unique_ptr>(cx);
  clear_table(cx);
  test_optional<std::optional>(cx);
  clear_table(cx);
  test_optional_fold<std::optional>(cx);
  clear_table(cx);
  test_variant_fold(cx);
}


void test_stream_to_factory_with_static_columns()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec("CREATE TEMP TABLE pqxx_stream_to(a integer, b varchar)").no_rows();

  auto stream{pqxx::stream_to::table(tx, {"pqxx_stream_to"}, {"a", "b"})};
  stream.write_values(3, "three");
  stream.complete();

  auto r{tx.exec("SELECT a, b FROM pqxx_stream_to").one_row()};
  PQXX_CHECK_EQUAL(r[0].as<int>(), 3, "Failed to stream_to a table.");
  PQXX_CHECK_EQUAL(
    r[1].as<std::string>(), "three",
    "Failed to stream_to a string to a table.");
}


void test_stream_to_factory_with_dynamic_columns()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec("CREATE TEMP TABLE pqxx_stream_to(a integer, b varchar)").no_rows();

  std::vector<std::string_view> columns{"a", "b"};
#if defined(PQXX_HAVE_CONCEPTS)
  auto stream{pqxx::stream_to::table(tx, {"pqxx_stream_to"}, columns)};
#else
  auto stream{pqxx::stream_to::raw_table(
    tx, cx.quote_table({"pqxx_stream_to"}), cx.quote_columns(columns))};
#endif
  stream.write_values(4, "four");
  stream.complete();

  auto r{tx.exec("SELECT a, b FROM pqxx_stream_to").one_row()};
  PQXX_CHECK_EQUAL(
    r[0].as<int>(), 4, "Failed to stream_to a table with dynamic columns.");
  PQXX_CHECK_EQUAL(
    r[1].as<std::string>(), "four",
    "Failed to stream_to a string to a table with dynamic columns.");
}


void test_stream_to_quotes_arguments()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::string const table{R"--(pqxx_Stream"'x)--"}, column{R"--(a'"b)--"};

  tx.exec(
      "CREATE TEMP TABLE " + tx.quote_name(table) + "(" +
      tx.quote_name(column) + " integer)")
    .no_rows();
  auto write{pqxx::stream_to::table(tx, {table}, {column})};
  write.write_values<int>(12);
  write.complete();

  PQXX_CHECK_EQUAL(
    tx.query_value<int>(
      "SELECT " + tx.quote_name(column) + " FROM " + tx.quote_name(table)),
    12, "Stream wrote wrong value.");
}


void test_stream_to_optionals()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec("CREATE TEMP TABLE pqxx_strings(key integer, value varchar)")
    .no_rows();

  auto stream{pqxx::stream_to::table(tx, {"pqxx_strings"}, {"key", "value"})};
  stream.write_values(1, std::optional<std::string>{});
  stream.write_values(2, std::optional<std::string_view>{});
  stream.write_values(3, std::optional<pqxx::zview>{});
  stream.write_values(4, std::optional<std::string>{"Opt str."});
  stream.write_values(5, std::optional<std::string_view>{"Opt sv."});
  stream.write_values(6, std::optional<pqxx::zview>{"Opt zv."});

  stream.write_values(7, std::shared_ptr<std::string>{});
  stream.write_values(8, std::shared_ptr<std::string_view>{});
  stream.write_values(9, std::shared_ptr<pqxx::zview>{});
  stream.write_values(10, std::make_shared<std::string>("Shared str."));
  stream.write_values(11, std::make_shared<std::string_view>("Shared sv."));
  stream.write_values(12, std::make_shared<pqxx::zview>("Shared zv."));

  stream.write_values(13, std::unique_ptr<std::string>{});
  stream.write_values(14, std::unique_ptr<std::string_view>{});
  stream.write_values(15, std::unique_ptr<pqxx::zview>{});
  stream.write_values(16, std::make_unique<std::string>("Uq str."));
  stream.write_values(17, std::make_unique<std::string_view>("Uq sv."));
  stream.write_values(18, std::make_unique<pqxx::zview>("Uq zv."));
  stream.complete();

  std::string nulls;
  for (auto [key] : tx.query<int>(
         "SELECT key FROM pqxx_strings WHERE value IS NULL ORDER BY key"))
    nulls += pqxx::to_string(key) + '.';
  PQXX_CHECK_EQUAL(
    nulls, "1.2.3.7.8.9.13.14.15.", "Unexpected list of nulls.");

  std::string values;
  for (auto [value] :
       tx.query<std::string>("SELECT value FROM pqxx_strings WHERE value IS "
                             "NOT NULL ORDER BY key"))
    values += value;
  PQXX_CHECK_EQUAL(
    values,
    "Opt str.Opt sv.Opt zv.Shared str.Shared sv.Shared zv.Uq str.Uq sv.Uq zv.",
    "Unexpected list of values.");
}


void test_stream_to_escaping()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec("CREATE TEMP TABLE foo (i integer, t varchar)").no_rows();

  // We'll check that streaming these strings to the database and querying them
  // back reproduces them faithfully.
  std::string_view const inputs[] = {
    ""sv,      "hello"sv,    "a\tb"sv, "a\nb"sv,
    "don't"sv, "\\\\\\''"sv, "\\N"sv,  "\\Nfoo"sv,
  };

  // Stream the input strings into the databsae.
  pqxx::stream_to out{pqxx::stream_to::table(tx, {"foo"}, {"i", "t"})};
  for (std::size_t i{0}; i < std::size(inputs); ++i)
    out.write_values(i, inputs[i]);
  out.complete();

  // Verify.
  auto outputs{tx.exec("SELECT i, t FROM foo ORDER BY i")};
  PQXX_CHECK_EQUAL(
    static_cast<std::size_t>(std::size(outputs)), std::size(inputs),
    "Wrong number of rows came back.");
  for (std::size_t i{0}; i < std::size(inputs); ++i)
  {
    int idx{static_cast<int>(i)};
    PQXX_CHECK_EQUAL(
      outputs[idx][0].as<std::size_t>(), i, "Unexpected index.");
    PQXX_CHECK_EQUAL(
      outputs[idx][1].as<std::string_view>(), inputs[i],
      "String changed in transit.");
  }
}


void test_stream_to_moves_into_optional()
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  tx.exec("CREATE TEMP TABLE foo (a integer)").no_rows();
  std::optional<pqxx::stream_to> org{
    std::in_place, pqxx::stream_to::table(tx, {"foo"}, {"a"})};
  org->write_values(1);
  auto copy{std::move(org)};
  copy->write_values(2);
  copy->complete();
  auto values{tx.exec("SELECT a FROM foo ORDER BY a").expect_rows(2)};
  PQXX_CHECK_EQUAL(
    values[0][0].as<int>(), 1, "Streaming results start off wrong.");
  PQXX_CHECK_EQUAL(values[1][0].as<int>(), 2, "Moved stream went wrong.");
}


void test_stream_to_empty_strings()
{
  // Reproduce #816: Streaming an array of 4 or more empty strings to a table
  // using stream_to crashes.
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  tx.exec("CREATE TEMP TABLE strs (list text[])").no_rows();
  std::vector<std::string> empties{"", "", "", ""};
  auto stream{pqxx::stream_to::table(tx, {"strs"})};
  stream.write_values(std::variant<std::vector<std::string>>{empties});
  stream.complete();
  tx.commit();
}


PQXX_REGISTER_TEST(test_stream_to);
PQXX_REGISTER_TEST(test_container_stream_to);
PQXX_REGISTER_TEST(test_stream_to_does_nonnull_optional);
PQXX_REGISTER_TEST(test_stream_to_factory_with_static_columns);
PQXX_REGISTER_TEST(test_stream_to_factory_with_dynamic_columns);
PQXX_REGISTER_TEST(test_stream_to_quotes_arguments);
PQXX_REGISTER_TEST(test_stream_to_optionals);
PQXX_REGISTER_TEST(test_stream_to_escaping);
PQXX_REGISTER_TEST(test_stream_to_moves_into_optional);
PQXX_REGISTER_TEST(test_stream_to_empty_strings);
} // namespace
