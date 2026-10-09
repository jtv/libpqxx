#include <pqxx/pqxx>

#include "helpers.hxx"


namespace
{
using pqxx::operator""_zv;


void test_statement_params(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::array<std::byte, 2> const bin{std::byte{'a'}, std::byte{'b'}};

  pqxx::params p, q;
  p.append();
  p.append("zview"_zv);
  q.append(bin);
  q.append(pqxx::bytes{});
  p.append(q);

  auto const res{tx.exec("SELECT $1, $2, $3, $4", p)};
  PQXX_CHECK(res.at(0).at(0).is_null());
  PQXX_CHECK_EQUAL(res.at(0).at(1).view(), "zview");
  PQXX_CHECK_EQUAL(res.at(0).at(2).view(), "ab");
  PQXX_CHECK(not res.at(0).at(3).is_null());
  PQXX_CHECK_EQUAL(res.at(0).at(3).view(), "");
}


// TODO: Split this up into smaller tests.
/// Test from PR #1250: fix binary std::optional etc. params.
/** One semantic change is that #1250 would _copy_ binary data passed in as a
 * view inside a `std::optional` or smart pointer into the `params`.  The
 * current implementation just keeps the pointer and expects the caller to keep
 * it valid.
 */
void test_statement_params_advanced(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  using binary_data = std::array<std::byte, 2>;
  binary_data const bin{std::byte{'a'}, std::byte{'b'}};
  pqxx::bytes bin2{std::byte{'b'}, std::byte{'c'}};
  pqxx::bytes_view bin3{bin2};

  // Test cases:
  // 1. General append usage.
  // 2. Append by value, view, reference (lifetime differs).
  // 3. "Maybe types," by value and by reference.
  //
  // There's a danger of dangling references:
  // * append(std::optional<const bytes>{std::move(value)});
  // * append(std::make_unique<bytes>(value));
  pqxx::params p;
  p.append();
  p.append("zview"_zv);

  {
    pqxx::params q;
    q.append(bin);
    p.append(q);
    // (End of q's lifetime.)
  }

  p.append(pqxx::bytes{});
  p.append(std::optional<pqxx::bytes>{});
  std::optional const v1{bin3};
  p.append(v1); // Object passed by reference, lifetime managed by caller.

  const std::optional<std::optional<pqxx::bytes_view>> v2{
    std::in_place, std::in_place, bin3};
  p.append(v2); // Passed by reference, lifetime managed by caller.

  auto const v3 = std::make_unique<pqxx::bytes_view>(bin3);
  p.append(v3); // Passed by reference, lifetime managed by caller.
  // Doesn't actually move, because there's no rvalue overload.
  p.append(std::move(v3));
  // Passed by value, params takes ownership:
  p.append(std::make_unique<pqxx::bytes_view>(bin3));

  auto const v4 = std::make_shared<pqxx::bytes_view>(bin3);
  p.append(v4); // Passed by reference, lifetime managed by caller.

  std::variant<pqxx::bytes_view, int> const v5{bin3};
  p.append(v5); // Passed by reference.
  // Gets converted to string inside the params:
  p.append(std::variant<pqxx::bytes_view, int>{42});
  {
    const auto c_params = p.make_c_params({});
    constexpr auto binary = static_cast<int>(pqxx::format::binary);
    constexpr auto text = static_cast<int>(pqxx::format::text);
    constexpr size_t size{0u};

    PQXX_CHECK_EQUAL(c_params.formats[2], binary);
    PQXX_CHECK(c_params.values[4] == nullptr);
    PQXX_CHECK_EQUAL(c_params.formats[5], binary);
    // Stored as view, so pointer remains the same.
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[5], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[6], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[6], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[7], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[7], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[8], binary);
    // This data gets copied inside the params.
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[8], size).data() != bin.data());
    PQXX_CHECK_EQUAL(c_params.formats[9], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[9], size).data() == bin2.data());

    PQXX_CHECK_EQUAL(c_params.formats[10], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[10], size).data() == bin2.data());
    PQXX_CHECK_EQUAL(c_params.formats[11], binary);
    PQXX_CHECK(
      pqxx::binary_cast(c_params.values[11], size).data() == bin2.data());

    PQXX_CHECK_EQUAL(c_params.formats[12], text);
  }

  auto const res{tx.exec(
    "SELECT $1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13", p)};
  auto const row = res.at(0);
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
PQXX_REGISTER_TEST(test_statement_params_advanced);
} // namespace
