#include "../test_helpers.hxx"
#include "../test_types.hxx"

#include <iostream>
#include <optional>

#include <pqxx/stream_to>


namespace
{
std::string truncate_sql_error(std::string const &what)
{
  auto trunc{what.substr(0, what.find('\n'))};
  if (trunc.size() > 64)
    trunc = trunc.substr(0, 61) + "...";
  return trunc;
}


void test_nonoptionals(pqxx::connection_base &connection)
{
  pqxx::work tx{connection};
  pqxx::stream_to inserter{tx, "stream_to_test"};
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

  auto r1{tx.exec1("SELECT * FROM stream_to_test WHERE number0 = 1234")};
  PQXX_CHECK_EQUAL(r1[0].as<int>(), 1234, "Read back wrong first int.");
  PQXX_CHECK_EQUAL(
    r1[4].as<std::string>(), "hello nonoptional world",
    "Read back wrong string.");
  PQXX_CHECK_EQUAL(r1[3].as<ipv4>(), ipv4(8, 8, 4, 4), "Read back wrong ip.");
  PQXX_CHECK_EQUAL(r1[5].as<bytea>(), binary, "Read back wrong bytera.");

  auto r2{tx.exec1("SELECT * FROM stream_to_test WHERE number0 = 5678")};
  PQXX_CHECK_EQUAL(r2[0].as<int>(), 5678, "Wrong int on second row.");
  PQXX_CHECK(r2[2].is_null(), "Field 2 was meant to be null.");
  PQXX_CHECK(r2[3].is_null(), "Field 3 was meant to be null.");
  PQXX_CHECK_EQUAL(r2[4].as<std::string>(), nonascii, "Wrong non-ascii text.");
  tx.commit();
}


void test_bad_null(pqxx::connection_base &connection)
{
  pqxx::work transaction{connection};
  pqxx::stream_to inserter{transaction, "stream_to_test"};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter << std::make_tuple(
      nullptr, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
      bytea{'\x00', '\x01', '\x02'});
    inserter.complete();
    transaction.commit();
    PQXX_CHECK_NOTREACHED("stream_from improperly inserted row");
  }
  catch (pqxx::sql_error const &e)
  {
    std::string what{e.what()};
    if (what.find("null value in column") == std::string::npos)
      throw;
    pqxx::test::expected_exception(
      "Could not insert row: " + truncate_sql_error(what));
  }
}


void test_too_few_fields(pqxx::connection_base &connection)
{
  pqxx::work transaction{connection};
  pqxx::stream_to inserter{transaction, "stream_to_test"};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter << std::make_tuple(1234, "now", 4321, ipv4{8, 8, 8, 8});
    inserter.complete();
    transaction.commit();
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


void test_too_many_fields(pqxx::connection_base &connection)
{
  pqxx::work transaction{connection};
  pqxx::stream_to inserter{transaction, "stream_to_test"};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  try
  {
    inserter << std::make_tuple(
      1234, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
      bytea{'\x00', '\x01', '\x02'}, 5678);
    inserter.complete();
    transaction.commit();
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


template<template<typename...> class O>
void test_optional(pqxx::connection_base &connection)
{
  pqxx::work tx{connection};
  pqxx::stream_to inserter{tx, "stream_to_test"};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  inserter << std::make_tuple(
    910, O<std::string>{pqxx::nullness<O<std::string>>::null()},
    O<int>{pqxx::nullness<O<int>>::null()},
    O<ipv4>{pqxx::nullness<O<ipv4>>::null()}, "\\N", bytea{});

  inserter.complete();
  tx.commit();
}


// As an alternative to a tuple, you can also insert a container.
void test_container_stream_to()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  tx.exec0("CREATE TEMP TABLE test_container(a integer, b integer)");

  pqxx::stream_to inserter{tx, "test_container"};

  inserter << std::vector{112, 244};
  inserter.complete();

  auto read{tx.exec1("SELECT * FROM test_container")};
  PQXX_CHECK_EQUAL(
    read[0].as<int>(), 112, "stream_to on container went wrong.");
  PQXX_CHECK_EQUAL(
    read[1].as<int>(), 244, "Second container field went wrong.");
  tx.commit();
}


void clear_table(pqxx::connection &conn)
{
  pqxx::work tx{conn};
  tx.exec0("DELETE FROM stream_to_test");
  tx.commit();
}


void test_stream_to()
{
  pqxx::connection conn;
  pqxx::work tx{conn};

  tx.exec0(
    "CREATE TEMP TABLE stream_to_test ("
    "number0 INT NOT NULL,"
    "ts1     TIMESTAMP NULL,"
    "number2 INT NULL,"
    "addr3   INET NULL,"
    "txt4    TEXT NULL,"
    "bin5    BYTEA NOT NULL"
    ")");
  tx.commit();

  test_nonoptionals(conn);
  clear_table(conn);
  test_bad_null(conn);
  clear_table(conn);
  test_too_few_fields(conn);
  clear_table(conn);
  test_too_many_fields(conn);
  clear_table(conn);
  test_optional<std::unique_ptr>(conn);
  clear_table(conn);
  test_optional<std::optional>(conn);
}


PQXX_REGISTER_TEST(test_stream_to);
PQXX_REGISTER_TEST(test_container_stream_to);
} // namespace
