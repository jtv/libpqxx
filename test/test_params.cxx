#include <pqxx/pqxx>

#include "helpers.hxx"

namespace
{
using pqxx::operator""_zv;


void test_statement_params(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  using binary_data = std::array<std::byte, 2>;
  binary_data const bin{std::byte{'a'}, std::byte{'b'}};
  pqxx::bytes bin2{std::byte{'b'}, std::byte{'c'}};
  pqxx::bytes_view bin3{bin2};

  // test cases:
  // 1. general append usage
  // 2. append by value, view, reference - lifetime is different
  // 3. append wrapped value(std:some_ptr, std::optional, std::variant) by
  // value and reference
  //
  // Potential dangling references:
  // append(std::optional<const bytes>{std::move(value)});
  // append(std::make_unique<bytes>{value});

  pqxx::params p;
  p.append();           // 0
  p.append("zview"_zv); // 1

  {
    pqxx::params q;
    q.append(bin);
    p.append(q); // 2
  }

  p.append(pqxx::bytes{});                // 3
  p.append(std::optional<pqxx::bytes>{}); // 4
  std::optional v1{bin3};
  p.append(v1); // 5, object passed by reference, lifetime is managed by user

  const std::optional<std::optional<pqxx::bytes_view>> v2{
    std::in_place, std::in_place, bin3};
  p.append(v2); // 6, same, lifetime is managed by user

  auto v3 = std::make_unique<pqxx::bytes_view>(bin3);
  p.append(v3); // 7, same, lifetime is managed by user
  // 8, rvalue reference, lifetime is managed by params
  p.append(std::move(v3));
  // 9, by value, lifetime is managed by params
  p.append(std::make_unique<pqxx::bytes_view>(bin3));

  auto v4 = std::make_shared<pqxx::bytes_view>(bin3);
  p.append(v4); // 10, same as 6

  std::variant<pqxx::bytes_view, int> v5{bin3};
  p.append(v5);                                      // 11, by reference
  p.append(std::variant<pqxx::bytes_view, int>{42}); // 12, converted to string
  {
    const auto c_params = p.make_c_params({});
    constexpr auto binary = static_cast<int>(pqxx::format::binary);
    constexpr auto text = static_cast<int>(pqxx::format::text);
    constexpr size_t size{0};

    PQXX_CHECK_EQUAL(c_params.formats[2], binary);
    PQXX_CHECK(c_params.values[4] == nullptr);
    PQXX_CHECK_EQUAL(c_params.formats[5], binary);
    // pointer is the same, bytes are stored by view
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[5], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[6], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[6], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[7], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[7], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[8], binary);
    // copy has been created
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[8], size).data() != bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[9], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[9], size).data() != bin2.data());

    PQXX_CHECK_EQUAL(c_params.formats[10], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[10], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[11], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[11], size).data() == bin2.data());

    PQXX_CHECK_EQUAL(c_params.formats[12], text);
  }

  auto const res{
    tx.exec("SELECT $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13", p)};
  const auto &row = res.at(0);
  PQXX_CHECK(row.at(0).is_null());
  PQXX_CHECK_EQUAL(row.at(1).view(), "zview");
  PQXX_CHECK_EQUAL(row.at(2).view(), "ab");
  PQXX_CHECK(not row.at(3).is_null());
  PQXX_CHECK_EQUAL(row.at(3).view(), "");
  PQXX_CHECK(row.at(4).is_null());
  PQXX_CHECK(not row.at(5).is_null());
  PQXX_CHECK_EQUAL(row.at(5).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(6).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(7).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(8).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(9).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(10).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(11).view(), "bc");
  PQXX_CHECK_EQUAL(row.at(12).view(), "42");
}


PQXX_REGISTER_TEST(test_statement_params);
} // namespace
