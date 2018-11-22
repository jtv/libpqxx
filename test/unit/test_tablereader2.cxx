#include "../test_helpers.hxx"
#include "test_types.hxx"

#include <pqxx/tablereader2>

#include <cstring>
#include <iostream>
#include <string>
#include <tuple>
#include <vector>

#if defined PQXX_HAVE_OPTIONAL
#include <optional>
#elif defined PQXX_HAVE_EXP_OPTIONAL
#include <experimental/optional>
#endif


namespace
{


void test_nonoptionals(pqxx::connection_base& connection)
{
    pqxx::work transaction{ connection };

    pqxx::tablereader2 extractor{
        transaction,
        "tablereader2_test"
    };

    PQXX_CHECK(extractor, "tablereader2 failed to initialize");

    std::tuple<
        int,
        std::string,
        int,
        ipv4,
        std::string,
        bytea
    > got_tuple;

    extractor >> got_tuple;
    PQXX_CHECK(extractor, "tablereader2 failed to read first row");
    PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 1234, "field value mismatch");
    // PQXX_CHECK_EQUAL(*std::get<1>(got_tuple), , "field value mismatch");
    PQXX_CHECK_EQUAL(std::get<2>(got_tuple), 4321, "field value mismatch");
    PQXX_CHECK_EQUAL(std::get<3>(got_tuple), (ipv4{8, 8, 8, 8}), "field value mismatch");
    PQXX_CHECK_EQUAL(std::get<4>(got_tuple), "hello world", "field value mismatch");
    PQXX_CHECK_EQUAL(std::get<5>(got_tuple), (bytea{'\x00', '\x01', '\x02'}), "field value mismatch");

    try
    {
        extractor >> got_tuple;
        PQXX_CHECK(!extractor, "tablereader2 improperly read second row");
    }
    catch (const pqxx::conversion_error &e)
    {
        pqxx::test::expected_exception(
            std::string{"Could not extract row: "} + e.what()
        );
    }

    std::tuple<
        int,
        std::string,
        std::nullptr_t,
        std::nullptr_t,
        std::string,
        bytea
    > got_tuple_nulls1;
    std::tuple<
        int,
        std::nullptr_t,
        std::nullptr_t,
        std::nullptr_t,
        std::string,
        bytea
    > got_tuple_nulls2;

    try
    {
        extractor >> got_tuple_nulls2;
        PQXX_CHECK(!extractor, "tablereader2 improperly read second row");
    }
    catch (const pqxx::conversion_error &e)
    {
        pqxx::test::expected_exception(
            std::string{"Could not extract row: "} + e.what()
        );
    }

    extractor >> got_tuple_nulls1;
    PQXX_CHECK(extractor, "tablereader2 failed to reentrantly read second row");
    extractor >> got_tuple_nulls2;
    PQXX_CHECK(extractor, "tablereader2 failed to reentrantly read third row");
    extractor >> got_tuple;
    PQXX_CHECK(!extractor, "tablereader2 failed to detect end of stream");

    extractor.complete();
}


#define ASSERT_FIELD_EQUAL(OPT, VAL) \
    PQXX_CHECK(static_cast<bool>(OPT), "unexpected null field"); \
    PQXX_CHECK_EQUAL(*OPT, VAL, "field value mismatch")
#define ASSERT_FIELD_NULL(OPT) \
    PQXX_CHECK(!static_cast<bool>(OPT), "expected null field")


template<template<typename...> class O>
void test_optional(pqxx::connection_base& connection)
{
    pqxx::work transaction{ connection };

    pqxx::tablereader2 extractor{
        transaction,
        "tablereader2_test"
    };

    PQXX_CHECK(extractor, "tablereader2 failed to initialize");

    std::tuple<
        int,
        O<std::string>,
        O<int        >,
        O<ipv4       >,
        O<std::string>,
        O<bytea      >
    > got_tuple;

    extractor >> got_tuple;
    PQXX_CHECK(extractor, "tablereader2 failed to read first row");
    PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 1234, "field value mismatch");
    PQXX_CHECK(static_cast<bool>(std::get<1>(got_tuple)), "unexpected null field");
    // PQXX_CHECK_EQUAL(*std::get<1>(got_tuple), , "field value mismatch");
    ASSERT_FIELD_EQUAL(std::get<2>(got_tuple), 4321);
    ASSERT_FIELD_EQUAL(std::get<3>(got_tuple), (ipv4{8, 8, 8, 8}));
    ASSERT_FIELD_EQUAL(std::get<4>(got_tuple), "hello world");
    ASSERT_FIELD_EQUAL(std::get<5>(got_tuple), (bytea{'\x00', '\x01', '\x02'}));

    extractor >> got_tuple;
    PQXX_CHECK(extractor, "tablereader2 failed to read second row");
    PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 5678, "field value mismatch");
    ASSERT_FIELD_EQUAL(std::get<1>(got_tuple), "2018-11-17 21:23:00");
    ASSERT_FIELD_NULL(std::get<2>(got_tuple));
    ASSERT_FIELD_NULL(std::get<3>(got_tuple));
    ASSERT_FIELD_EQUAL(std::get<4>(got_tuple), "こんにちは");
    ASSERT_FIELD_EQUAL(std::get<5>(got_tuple), (bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'}));

    extractor >> got_tuple;
    PQXX_CHECK(extractor, "tablereader2 failed to read third row");
    PQXX_CHECK_EQUAL(std::get<0>(got_tuple), 910, "field value mismatch");
    ASSERT_FIELD_NULL(std::get<1>(got_tuple));
    ASSERT_FIELD_NULL(std::get<2>(got_tuple));
    ASSERT_FIELD_NULL(std::get<3>(got_tuple));
    ASSERT_FIELD_EQUAL(std::get<4>(got_tuple), "\\N");
    ASSERT_FIELD_EQUAL(std::get<5>(got_tuple), bytea{});

    extractor >> got_tuple;
    PQXX_CHECK(!extractor, "tablereader2 failed to detect end of stream");

    extractor.complete();
}


void test_tablereader2(pqxx::transaction_base &nontrans)
{
    auto& connection = nontrans.conn();
    nontrans.abort();

    pqxx::work transaction{connection};
    transaction.exec(
        "CREATE TEMP TABLE tablereader2_test ("
        "number0 INT NOT NULL,"
        "ts1     TIMESTAMP NULL,"
        "number2 INT NULL,"
        "addr3   INET NULL,"
        "txt4    TEXT NULL,"
        "bin5    BYTEA NOT NULL"
        ")"
    );
    transaction.exec_params(
        "INSERT INTO tablereader2_test VALUES ($1,$2,$3,$4,$5,$6)",
        1234,
        "now",
        4321,
        ipv4{8, 8, 8, 8},
        "hello world",
        bytea{'\x00', '\x01', '\x02'}
    );
    transaction.exec_params(
        "INSERT INTO tablereader2_test VALUES ($1,$2,$3,$4,$5,$6)",
        5678,
        "2018-11-17 21:23:00",
        nullptr,
        nullptr,
        "こんにちは",
        bytea{'f', 'o', 'o', ' ', 'b', 'a', 'r', '\0'}
    );
    transaction.exec_params(
        "INSERT INTO tablereader2_test VALUES ($1,$2,$3,$4,$5,$6)",
        910,
        nullptr,
        nullptr,
        nullptr,
        "\\N",
        bytea{}
    );
    transaction.commit();

    test_nonoptionals(connection);
    std::cout << "testing `std::unique_ptr` as optional...\n";
    test_optional<std::unique_ptr>(connection);
    std::cout << "testing `custom_optional` as optional...\n";
    test_optional<custom_optional>(connection);
    #if defined PQXX_HAVE_OPTIONAL
    std::cout << "testing `std::optional` as optional...\n";
    test_optional<std::optional>(connection);
    #elif defined PQXX_HAVE_EXP_OPTIONAL
    std::cout << "testing `std::experimental::optional` as optional...\n";
    test_optional<std::experimental::optional>(connection);
    #endif
}


} // namespace


PQXX_REGISTER_TEST_T(test_tablereader2, pqxx::nontransaction)
