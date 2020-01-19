#include "../test_helpers.hxx"
#include "../test_types.hxx"

#include <iostream>
#include <optional>

#include <pqxx/stream_to>


namespace
{
std::string truncate_sql_error(std::string const &what)
{
  auto trunc = what.substr(0, what.find('\n'));
  if (trunc.size() > 64)
    trunc = trunc.substr(0, 61) + "...";
  return trunc;
}


void test_nonoptionals(pqxx::connection_base &connection)
{
  pqxx::work transaction{connection};
  pqxx::stream_to inserter{transaction, "stream_to_test"};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  inserter << std::make_tuple(
    1234, "now", 4321, ipv4{8, 8, 8, 8}, "hello world",
    bytea{'\x00', '\x01', '\x02'});
  inserter << std::make_tuple(
    5678, "2018-11-17 21:23:00", nullptr, nullptr, "こんにちは",
    bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'});
  inserter << std::make_tuple(910, nullptr, nullptr, nullptr, "\\N", bytea{});

  inserter.complete();
  transaction.commit();
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
  pqxx::work transaction{connection};
  pqxx::stream_to inserter{transaction, "stream_to_test"};
  PQXX_CHECK(inserter, "stream_to failed to initialize");

  inserter << std::make_tuple(
    910, O<std::string>{pqxx::nullness<O<std::string>>::null()},
    O<int>{pqxx::nullness<O<int>>::null()},
    O<ipv4>{pqxx::nullness<O<ipv4>>::null()}, "\\N", bytea{});

  inserter.complete();
  transaction.commit();
}


void test_stream_to()
{
  pqxx::connection conn;
  pqxx::work tx{conn};

  tx.exec(
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
  test_bad_null(conn);
  test_too_few_fields(conn);
  test_too_many_fields(conn);
  std::cout << "testing `std::unique_ptr` as optional...\n";
  test_optional<std::unique_ptr>(conn);
  std::cout << "testing `std::optional` as optional...\n";
  test_optional<std::optional>(conn);
}


PQXX_REGISTER_TEST(test_stream_to);
} // namespace
