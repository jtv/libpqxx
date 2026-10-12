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


void test_params_converts_most_types_to_string(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  int const n{tctx.make_num()};
  double const d{tctx.make_float_num<double>() / 2};
  std::vector<int> const a{tctx.make_num(), tctx.make_num()};
  int const bottom{tctx.make_num(10000)};
  int const top{bottom + tctx.make_num(100) + 1};
  pqxx::range<int> const r{
    pqxx::inclusive_bound<int>{bottom}, pqxx::exclusive_bound<int>{top}};
  std::string const s{tctx.make_name()};

  // This is how applications SHOULD use params: insert variables, but leave
  // the variables in place and unchanged until after the params object is no
  // longer needed.
  pqxx::row const out{
    tx.exec("SELECT $1, $2, $3, $4, $5", pqxx::params{n, d, a, r, s})
      .one_row()};

  // These parameters get passed into the SQL statement, as you'd expect.
  PQXX_CHECK_EQUAL(out.at(0).as<int>(), n);
  PQXX_CHECK_BOUNDS(out.at(1).as<double>(), d - 10, d + 10);
  PQXX_CHECK_EQUAL(out.at(2).as<std::vector<int>>(), a);
  PQXX_CHECK_EQUAL(out.at(3).as<pqxx::range<int>>(), r);
  PQXX_CHECK_EQUAL(out.at(4).as<std::string>(), s);

  // Whitebox test.  Actual applications using libpqxx MUST NOT do this.  It
  // gets too close to implementation details so may change in the future.
  // It works differently for certain other types.
  //
  // If we add these types of variables to a params, then change the values of
  // the variables, the params still contains the values as we inserted them.
  // Internally, these values get converted to strings immediately during
  // insertion.  It does not store any kind of reference to them.
  pqxx::row out2;
  {
    int n2{n};
    double d2{d};
    std::vector<int> a2{a};
    pqxx::range<int> r2{r};
    std::string s2{s};

    pqxx::params const p{n2, d2, a2, r2, s2};

    // Change the variables that we inserted, to show that this does not affect
    // the outcome (for these types).
    --n2;
    d2 -= 1000;
    --a2.at(0);
    a2.push_back(tctx.make_num());
    r2 = pqxx::range<int>{};
    s2 = tctx.make_name();

    // (Check that we've really changed all these variables.  And not
    // coincidentally, make static analysis tools shut up about the changes
    // never getting used anywhere.)
    PQXX_CHECK_NOT_EQUAL(n2, n);
    PQXX_CHECK_LESS(d2, d);
    PQXX_CHECK_NOT_EQUAL(a2, a);
    PQXX_CHECK_NOT_EQUAL(r2, r);
    PQXX_CHECK_NOT_EQUAL(s2, s);

    // Now use the params object we populated with these variables before we
    // changed them.
    out2 = tx.exec("SELECT $1, $2, $3, $4, $5", p).one_row();
  }

  // All the variables we appended to p have since changed, but the params
  // still contain the original values.  Do not rely on this, it's just a
  // whitebox test to establish that params works as expected, *internally.*
  PQXX_CHECK_EQUAL(out2.at(0).as<int>(), n);
  PQXX_CHECK_BOUNDS(out2.at(1).as<double>(), d - 10, d + 10);
  PQXX_CHECK_EQUAL(out2.at(2).as<std::vector<int>>(), a);
  PQXX_CHECK_EQUAL(out2.at(3).as<pqxx::range<int>>(), r);
  PQXX_CHECK_EQUAL(out.at(4).as<std::string>(), s);
}


void test_params_stores_specific_types_as_views(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::byte const bx{tctx.make_byte()}, by{tctx.make_byte()},
    bz{tctx.make_byte()};
  pqxx::bytes const b{bx, by, bz};
  std::string const s{tctx.make_name("one")};

  // This is how applications SHOULD use params: insert variabes, but leave
  // the variables in place and unchanged until after the params object is no
  // longer needed.
  pqxx::row const out{
    tx.exec(
        "SELECT $1::bytea, $2::bytea, $3",
        pqxx::params{b, pqxx::bytes_view{b}, std::string_view{s}})
      .one_row()};

  // These parameters get passed into the SQL statement, as you'd expect.
  pqxx::test::check_equal_bin(out.at(0).as<pqxx::bytes>(), b);
  pqxx::test::check_equal_bin(out.at(1).as<pqxx::bytes>(), b);
  PQXX_CHECK_EQUAL(out.at(2).view(), s);

  // Whitebox test.  Actual applications using libpqxx MUST NOT do this.  It
  // gets too close to implementation details so may change in the future.
  // It also works differently for other types.
  //
  // If we add these types of variables to a params, but change the data inside
  // them before using the params, then what goes out to the database server as
  // parameters is the changed data.
  pqxx::row out2;
  {
    pqxx::bytes b2{b};
    std::string s2{s};

    pqxx::params const p{b2, pqxx::bytes_view{b2}, pqxx::zview{s2}};

    // Change the data we've already inserted into p.
    std::exchange(b2[0], b2[2]);
    b2[2] = tctx.make_byte();
    s2 = tctx.make_name("two");

    // Now pass p to a statement.  What goes out to te server will be the
    // changed values, not the values as they were when we inserted them into
    // p.
    out2 = tx.exec("SELECT $1::bytea, $2::bytea, $3", p).one_row();

    pqxx::test::check_equal_bin(out2.at(0).as<pqxx::bytes>(), b2);
    pqxx::test::check_equal_bin(out2.at(1).as<pqxx::bytes>(), b2);
    PQXX_CHECK_EQUAL(out2.at(2).as<std::string>(), s2);
  }
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
PQXX_REGISTER_TEST(test_params_converts_most_types_to_string);
PQXX_REGISTER_TEST(test_params_stores_specific_types_as_views);
PQXX_REGISTER_TEST(test_statement_params_advanced);
} // namespace
