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
  std::string_view const query{"SELECT $1, $2, $3, $4, $5, $6::bytea"};
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
  pqxx::bytes const b{tctx.make_bytes(10)};

  // This is how applications SHOULD use params: insert variables, but leave
  // the variables in place and unchanged until after the params object is no
  // longer needed.
  pqxx::row const out{
    tx.exec(query, pqxx::params{n, d, a, r, s, pqxx::bytes{b}}).one_row()};

  // These parameters get passed into the SQL statement, as you'd expect.
  PQXX_CHECK_EQUAL(out.at(0).as<int>(), n);
  PQXX_CHECK_BOUNDS(out.at(1).as<double>(), d - 10, d + 10);
  PQXX_CHECK_EQUAL(out.at(2).as<std::vector<int>>(), a);
  PQXX_CHECK_EQUAL(out.at(3).as<pqxx::range<int>>(), r);
  PQXX_CHECK_EQUAL(out.at(4).as<std::string>(), s);
  pqxx::test::check_equal_bin(out.at(5).as<pqxx::bytes>(), b);

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
    pqxx::bytes b2{b};

    // There's no way to pass a pqxx::bytes to params by value, because that'd
    // be a performance trap.  You can, however, pass one in as an rvalue
    // reference.  And so we run the bytes values through an extra copy step
    // just for this test.
    pqxx::params const p{n2, d2, a2, r2, s2, pqxx::bytes{b2}};

    // Change the variables that we inserted, to show that this does not affect
    // the outcome (for these types).
    --n2;
    d2 -= 1000;
    --a2.at(0);
    a2.push_back(tctx.make_num());
    r2 = pqxx::range<int>{};
    s2 = tctx.make_name();
    b2 = tctx.make_bytes();

    // (Check that we've really changed all these variables.  And not
    // coincidentally, make static analysis tools shut up about the changes
    // never getting used anywhere.)
    PQXX_CHECK_NOT_EQUAL(n2, n);
    PQXX_CHECK_LESS(d2, d);
    PQXX_CHECK_NOT_EQUAL(a2, a);
    PQXX_CHECK_NOT_EQUAL(r2, r);
    PQXX_CHECK_NOT_EQUAL(s2, s);
    pqxx::test::check_not_equal_bin(b2, b);

    // Now use the params object we populated with these variables before we
    // changed them.
    out2 = tx.exec(query, p).one_row();
  }

  // All the variables we appended to p have since changed, but the params
  // still contain the original values.  Do not rely on this, it's just a
  // whitebox test to establish that params works as expected, *internally.*
  PQXX_CHECK_EQUAL(out2.at(0).as<int>(), n);
  PQXX_CHECK_BOUNDS(out2.at(1).as<double>(), d - 10, d + 10);
  PQXX_CHECK_EQUAL(out2.at(2).as<std::vector<int>>(), a);
  PQXX_CHECK_EQUAL(out2.at(3).as<pqxx::range<int>>(), r);
  PQXX_CHECK_EQUAL(out.at(4).as<std::string>(), s);
  pqxx::test::check_equal_bin(out.at(5).as<pqxx::bytes>(), b);
}


void test_params_stores_specific_types_as_views(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::string_view const query{"SELECT $1::bytea, $2::text"};
  pqxx::bytes b{tctx.make_bytes(10)};
  std::string s{tctx.make_name("text param")};

  // This is how applications SHOULD use params: insert variabes, but leave
  // the variables in place and unchanged until after the params object is no
  // longer needed.
  pqxx::params const p{pqxx::binary_cast(b), pqxx::zview{s}};
  pqxx::row const out{tx.exec(query, p).one_row()};
  pqxx::test::check_equal_bin(out.at(0).as<pqxx::bytes>(), b);
  PQXX_CHECK_EQUAL(out.at(1).view(), s);

  // Whitebox test follows.  Actual applications using libpqxx MUST NOT do
  // this.  It gets too close to implementation details so may change in the
  // future.  It also works differently for other types.
  //
  // If we add these types of variables to a params, but change the data inside
  // them before using the params, then what goes out to the database server as
  // parameters is the changed data.
  //
  // (Be careful not to let b and s re-allocate their buffers internally, since
  // p stores views referring to the data in those buffers.)
  {
    for (std::size_t i{0}; i < std::size(b); ++i) b.at(i) = tctx.make_byte();
  }
  {
    for (std::size_t i{0}; i < std::size(s); ++i)
      s.at(i) = tctx.make_ascii_char();
  }

  // Now pass p to a statement.  What goes out to the server will be the
  // changed values, not the values as they were when we inserted them into p.
  auto const out2{tx.exec(query, p).one_row()};

  pqxx::test::check_equal_bin(out2.at(0).as<pqxx::bytes>(), b);
  PQXX_CHECK_EQUAL(out2.at(1).as<std::string>(), s);
}


void test_params_sees_through_maybe_types(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  int const n{tctx.make_num()};
  std::string const s{tctx.make_name()};
  pqxx::bytes b{tctx.make_bytes()};

  auto const query{"SELECT $1, $2, $3::bytea"};

  {
    pqxx::row const r0{tx.exec(query, pqxx::params{n, s, b}).one_row()};
    PQXX_CHECK_EQUAL(r0.at(0).as<int>(), n);
    PQXX_CHECK_EQUAL(r0.at(1).view(), s);
    pqxx::test::check_equal_bin(r0.at(2).as<pqxx::bytes>(), b);
  }

  {
    pqxx::row const r1{
      tx.exec(
          query,
          pqxx::params{std::optional{n}, std::optional{s}, std::optional{b}})
        .one_row()};
    PQXX_CHECK_EQUAL(r1.at(0).as<int>(), n);
    PQXX_CHECK_EQUAL(r1.at(1).view(), s);
    pqxx::test::check_equal_bin(r1.at(2).as<pqxx::bytes>(), b);
  }

  {
    pqxx::row const r2{
      tx.exec(
          query,
          pqxx::params{
            std::optional{std::make_shared<int>(n)},
            std::optional{std::make_shared<std::string>(s)},
            std::optional{std::make_shared<pqxx::bytes_view>(b)}})
        .one_row()};
    PQXX_CHECK_EQUAL(r2.at(0).as<int>(), n);
    PQXX_CHECK_EQUAL(r2.at(1).view(), s);
    pqxx::test::check_equal_bin(r2.at(2).as<pqxx::bytes>(), b);
  }
}


void test_statement_params_recognises_null_maybes(pqxx::test::context &tctx)

{
  pqxx::connection cx;
  pqxx::work tx{cx};

  // Helper: Pass param to a SELECT statement, query it as a std::optional of
  // int.
  auto const query_opt{[&tx](auto const &param) {
    return tx.query_value<std::optional<int>>(
      "SELECT $1", pqxx::params{param});
  }};

  int const n{tctx.make_num()};

  PQXX_CHECK(query_opt(std::optional<int>{n}).has_value());
  // NOLINTNEXTLINE(bugprone-unchecked-optional-access)
  PQXX_CHECK_EQUAL(*query_opt(std::optional<int>{n}), n);

  PQXX_CHECK(not query_opt(std::optional<int>{}).has_value());
  PQXX_CHECK(not query_opt(std::shared_ptr<int>{}).has_value());
  PQXX_CHECK(not query_opt(std::unique_ptr<int>{}).has_value());

  // The params can also "see through" multiple layers of nested "maybe-type"
  // objects.
  PQXX_CHECK(not query_opt(std::optional<std::unique_ptr<int>>{}).has_value());
  PQXX_CHECK(not query_opt(std::unique_ptr<std::optional<int>>{}).has_value());

  // In situations where one maybe-type object is nested inside another, we
  // can get a null even if the top-level one is not blank.
  PQXX_CHECK(
    not query_opt(std::optional<std::unique_ptr<int>>{std::unique_ptr<int>{}})
          .has_value());
  PQXX_CHECK(
    not query_opt(std::make_shared<std::optional<int>>(std::optional<int>{}))
          .has_value());
}


PQXX_REGISTER_TEST(test_statement_params);
PQXX_REGISTER_TEST(test_params_converts_most_types_to_string);
PQXX_REGISTER_TEST(test_params_stores_specific_types_as_views);
PQXX_REGISTER_TEST(test_params_sees_through_maybe_types);
PQXX_REGISTER_TEST(test_statement_params_recognises_null_maybes);
} // namespace
