#include "../test_helpers.hxx"

#include <pqxx/tablereader2>

#include <cstdint>
#include <cstring>
#include <exception>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <tuple>
#include <vector>

#if defined PQXX_HAVE_OPTIONAL
#include <optional>
#elif defined PQXX_HAVE_EXP_OPTIONAL
#include <experimental/optional>
#endif


namespace // Custom types for testing
{

union ipv4
{
    uint32_t as_int;
    unsigned char as_bytes[4];

    ipv4() : as_int{0x00} {}
    ipv4(const ipv4& o) : as_int{o.as_int} {}
    ipv4(uint32_t i) : as_int{i} {}
    ipv4(
        unsigned char b1,
        unsigned char b2,
        unsigned char b3,
        unsigned char b4
    ) : as_bytes{b1, b2, b3, b4} {}
    bool operator ==(const ipv4 &o) const { return as_int == o.as_int; }
};

using bytea = std::vector<unsigned char>;

template<typename T> class custom_optional
{
    // Very basic, only has enough features for testing
private:
    union
    {
        void *_;
        T value;
    };
    bool has_value;
public:
    custom_optional() : has_value{false}, _{} {}
    custom_optional(std::nullptr_t) : has_value{false}, _{} {}
    custom_optional(const custom_optional<T> &o) : has_value{o.has_value}, _{}
    {
        if (has_value) new(&value) T(o.value);
    }
    custom_optional(const T& v) : has_value{true}, value{v} {}
    ~custom_optional()
    {
        if (has_value) value.~T();
    }
    explicit operator bool() { return has_value; }
    T &operator *()
    {
        if (has_value) return value;
        else throw std::logic_error{"bad optional access"};
    }
    const T &operator *() const
    {
        if (has_value) return value;
        else throw std::logic_error{"bad optional access"};
    }
    custom_optional<T> &operator =(const custom_optional<T> &o)
    {
        if (&o == this) return *this;
        if (has_value && o.has_value)
            value = o.value;
        else if (has_value) value.~T();
        has_value = o.has_value;
        if (has_value) new(&value) T(o.value);
        return *this;
    }
    custom_optional<T> &operator =(std::nullptr_t)
    {
        if (has_value) value.~T();
        has_value = false;
        return *this;
    }
};

}


namespace pqxx // libpqxx support for test types
{

template<> struct string_traits<ipv4>
{
    using subject_type = ipv4;

    static constexpr const char* name() noexcept
    {
        return "ipv4";
    }

    static constexpr bool has_null() noexcept { return false; }

    static bool is_null(const subject_type &) { return false; }

    [[noreturn]] static subject_type null()
    {
        internal::throw_null_conversion(name());
    }

    static void from_string(const char str[], subject_type &ts)
    {
        if (!str)
            internal::throw_null_conversion(name());
        std::regex ipv4_regex{
            "(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})\\.(\\d{1,3})"
        };
        std::smatch match;
        // Need non-temporary for `std::regex_match()`
        std::string sstr{str};
        if (!std::regex_match(sstr, match, ipv4_regex) || match.size() != 5)
            throw std::runtime_error{
                "invalid ipv4 format: " + std::string{str}
            };
        try
        {
            for (std::size_t i{0}; i < 4; ++i)
                ts.as_bytes[i] = std::stoi(match[i+1]);
        }
        catch (const std::invalid_argument&)
        {
            throw std::runtime_error{
                "invalid ipv4 format: " + std::string{str}
            };
        }
        catch (const std::out_of_range&)
        {
            throw std::runtime_error{
                "invalid ipv4 format: " + std::string{str}
            };
        }
    }

    static std::string to_string(const subject_type &ts)
    {
        return (
              std::to_string(static_cast<int>(ts.as_bytes[0]))
            + "."
            + std::to_string(static_cast<int>(ts.as_bytes[1]))
            + "."
            + std::to_string(static_cast<int>(ts.as_bytes[2]))
            + "."
            + std::to_string(static_cast<int>(ts.as_bytes[3]))
        );
    }
};

template<> struct string_traits<bytea>
{
private:
    static constexpr unsigned char from_hex(char c)
    {
        if (c >= '0' && c <= '9')
            return c - '0';
        else if (c >= 'a' || c <= 'f')
            return c - 'a' + 10;
        else if (c >= 'A' || c <= 'F')
            return c - 'A' + 10;
        else
            throw std::range_error{
                "character with value "
                + std::to_string(static_cast<unsigned int>(c))
                + " ("
                + std::string{c}
                + ") out-of-range for hexadecimal"
            };
    }
    static constexpr unsigned char from_hex(char c1, char c2)
    {
        return (from_hex(c1) << 4) | (from_hex(c2) & 0x0F);
    }

public:
    using subject_type = bytea;

    static constexpr const char* name() noexcept
    {
        return "bytea";
    }

    static constexpr bool has_null() noexcept { return false; }

    static bool is_null(const subject_type &) { return false; }

    [[noreturn]] static subject_type null()
    {
        internal::throw_null_conversion( name() );
    }

    static void from_string(const char str[], subject_type& bs)
    {
        if (!str)
            internal::throw_null_conversion(name());
        auto len = strlen(str);
        if (len % 2 || len < 2 || str[0] != '\\' || str[1] != 'x')
            throw std::runtime_error{
                "invalid bytea format: " + std::string{str}
            };
        bs.clear();
        for (std::size_t i{2}; i < len; /**/)
        {
            bs.emplace_back(from_hex(str[i], str[i + 1]));
            i += 2;
        }
    }

    static std::string to_string(const subject_type &bs)
    {
        std::stringstream s;
        s << "\\x" << std::hex;
        for (auto b : bs)
            s
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned int>(b)
            ;
        return s.str();
    }
};

}


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
