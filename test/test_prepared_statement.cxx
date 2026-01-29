#include <pqxx/transaction>

#include "helpers.hxx"

// Test program for libpqxx.  Define and use prepared statements.

#define COMPARE_RESULTS(name, lhs, rhs)                                       \
  PQXX_CHECK_EQUAL(                                                           \
    pqxx::to_string(rhs), pqxx::to_string(lhs),                               \
    "Executing " name " as prepared statement yields different results.");

namespace
{
using namespace std::literals;


template<typename T> std::string stringize(pqxx::transaction_base &t, T i)
{
  return stringize(t, pqxx::to_string(i));
}


// Substitute variables in raw query.  This is not likely to be very robust,
// but it should do for just this test.  The main shortcomings are escaping,
// and not knowing when to quote the variables.
// Note we do the replacement backwards (meaning forward_only iterators won't
// do!) to avoid substituting e.g. "$12" as "$1" first.
template<typename ITER>
std::string
subst(pqxx::transaction_base &t, std::string q, ITER patbegin, ITER patend)
{
  ptrdiff_t i{distance(patbegin, patend)};
  for (ITER arg{patend}; i > 0; --i)
  {
    --arg;
    std::string const marker{"$" + pqxx::to_string(i)},
      var{stringize(t, *arg)};
    std::string::size_type const msz{std::size(marker)};
    while (pqxx::str_contains(q, marker)) q.replace(q.find(marker), msz, var);
  }
  return q;
}


template<typename CNTNR>
std::string
subst(pqxx::transaction_base &t, std::string const &q, CNTNR const &patterns)
{
  return subst(t, q, std::begin(patterns), std::end(patterns));
}


void test_registration_and_invocation(pqxx::test::context &)
{
  constexpr auto count_to_5{"SELECT * FROM generate_series(1, 5)"};

  pqxx::connection cx;
  pqxx::work tx1{cx};

  // Prepare a simple statement.
  tx1.conn().prepare("CountToFive", count_to_5);

  // The statement returns exactly what you'd expect.
  COMPARE_RESULTS(
    "CountToFive", tx1.exec(pqxx::prepped{"CountToFive"}),
    tx1.exec(count_to_5));

  // Re-preparing it is an error.
  PQXX_CHECK_THROWS(
    tx1.conn().prepare("CountToFive", count_to_5), pqxx::sql_error);

  tx1.abort();
  pqxx::work tx2{cx};

  // Executing a nonexistent prepared statement is also an error.
  PQXX_CHECK_THROWS(
    tx2.exec(pqxx::prepped{"NonexistentStatement"}), pqxx::sql_error);
}


void test_basic_args(pqxx::test::context &)
{
  pqxx::connection cx;
  cx.prepare("EchoNum", "SELECT $1::int");
  pqxx::work tx{cx};
  auto r{tx.exec(pqxx::prepped{"EchoNum"}, 7)};
  PQXX_CHECK_EQUAL(std::size(r), 1);
  PQXX_CHECK_EQUAL(std::size(r.front()), 1);
  PQXX_CHECK_EQUAL(r.one_field().as<int>(), 7);

  auto rw{tx.exec(pqxx::prepped{"EchoNum"}, 8).one_row()};
  PQXX_CHECK_EQUAL(std::size(rw), 1);
  PQXX_CHECK_EQUAL(rw[0].as<int>(), 8);
}


void test_multiple_params(pqxx::test::context &)
{
  pqxx::connection cx;
  cx.prepare("CountSeries", "SELECT * FROM generate_series($1::int, $2::int)");
  pqxx::work tx{cx};
  auto r{tx.exec(pqxx::prepped{"CountSeries"}, pqxx::params{tx, 7, 10})
           .expect_rows(4)};
  PQXX_CHECK_EQUAL(std::size(r), 4);
  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 7);
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 10);

  cx.prepare("Reversed", "SELECT * FROM generate_series($2::int, $1::int)");
  r =
    tx.exec(pqxx::prepped{"Reversed"}, pqxx::params{tx, 8, 6}).expect_rows(3);
  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 6);
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 8);
}


void test_nulls(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  cx.prepare("EchoStr", "SELECT $1::varchar");
  auto rw{
    tx.exec(pqxx::prepped{"EchoStr"}, pqxx::params{tx, nullptr}).one_row()};
  PQXX_CHECK(rw.front().is_null());

  char const *n{nullptr};
  rw = tx.exec(pqxx::prepped{"EchoStr"}, pqxx::params{tx, n}).one_row();
  PQXX_CHECK(rw.front().is_null());
}


void test_strings(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  cx.prepare("EchoStr", "SELECT $1::varchar");
  auto rw{
    tx.exec(pqxx::prepped{"EchoStr"}, pqxx::params{tx, "foo"}).one_row()};
  PQXX_CHECK_EQUAL(rw.front().as<std::string>(), "foo");

  char const nasty_string[]{R"--('\"\)--"};
  rw = tx.exec(pqxx::prepped{"EchoStr"}, pqxx::params{tx, nasty_string})
         .one_row();
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), std::string(std::data(nasty_string)));

  rw = tx.exec(
           pqxx::prepped{"EchoStr"},
           pqxx::params{tx, std::string{std::data(nasty_string)}})
         .one_row();
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), std::string(std::data(nasty_string)));

  char nonconst[]{"non-const C string"};
  rw = tx.exec(pqxx::prepped{"EchoStr"}, pqxx::params{tx, nonconst}).one_row();
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), std::string(std::data(nonconst)));
}


void test_binary(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  cx.prepare("EchoBin", "SELECT $1::bytea");
  constexpr char raw_bytes[]{"Binary\0bytes'\"with\tweird\xff bytes"};
  std::string const input{std::data(raw_bytes), std::size(raw_bytes)};

  {
    // C++23: Initialise as bytes{std::from_range_t, raw_bytes)?
    pqxx::bytes bytes;
    for (char c : raw_bytes) bytes.push_back(static_cast<std::byte>(c));

    auto bp{
      tx.exec(pqxx::prepped{"EchoBin"}, pqxx::params{tx, bytes}).one_row()};
    auto bval{bp[0].as<pqxx::bytes>()};
    PQXX_CHECK_EQUAL(
      (std::string_view{
        reinterpret_cast<char const *>(std::data(bval)), std::size(bval)}),
      input);
  }

  // Now try it with a complex type that ultimately uses the conversions of
  // pqx::bytes, but complex enough that the call may convert the data to a
  // text string on the libpqxx side.  Which would be okay, except of course
  // it's likely to be slower.

  {
    // C++23: Initialise as data{std::from_range_t, raw_bytes)?
    pqxx::bytes data;
    for (char c : raw_bytes) data.push_back(static_cast<std::byte>(c));

    auto ptr{std::make_shared<pqxx::bytes>(data)};
    auto rp{
      tx.exec(pqxx::prepped{"EchoBin"}, pqxx::params{tx, ptr}).one_row()};
    auto pval{rp[0].as<pqxx::bytes>()};
    PQXX_CHECK_EQUAL(
      (std::string_view{
        reinterpret_cast<char const *>(std::data(pval)), std::size(pval)}),
      input);
  }

  {
    // C++23: Initialise as {std::in_place, std::from_range_t, raw_bytes}?
    std::vector<std::byte> data;
    for (char c : raw_bytes) data.push_back(static_cast<std::byte>(c));

    auto opt{std::optional<pqxx::bytes>{std::in_place, data}};
    auto op{
      tx.exec(pqxx::prepped{"EchoBin"}, pqxx::params{tx, opt}).one_row()};
    auto oval{op[0].as<pqxx::bytes>()};
    PQXX_CHECK_EQUAL(
      (std::string_view{
        reinterpret_cast<char const *>(std::data(oval)), std::size(oval)}),
      input);
  }

  // By the way, it doesn't have to be a pqxx::bytes.  Any contiguous range
  // will do.
  {
    std::vector<std::byte> const data{std::byte{'x'}, std::byte{'v'}};
    auto op{
      tx.exec(pqxx::prepped{"EchoBin"}, pqxx::params{tx, data}).one_row()};
    auto oval{op[0].as<pqxx::bytes>()};
    PQXX_CHECK_EQUAL(std::size(oval), 2u);
    PQXX_CHECK_EQUAL(static_cast<int>(oval[0]), int('x'));
    PQXX_CHECK_EQUAL(static_cast<int>(oval[1]), int('v'));
  }
}


void test_params(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  cx.prepare("Concat2Numbers", "SELECT 10 * $1 + $2");
  std::vector<int> const values{3, 9};
  pqxx::params params;
  params.reserve(std::size(values));
  params.append_multi(values);

  auto const rw39{
    tx.exec(pqxx::prepped{"Concat2Numbers"}, pqxx::params{tx, params})
      .one_row()};
  PQXX_CHECK_EQUAL(rw39.front().as<int>(), 39);

  cx.prepare("Concat4Numbers", "SELECT 1000*$1 + 100*$2 + 10*$3 + $4");
  auto const rw1396{
    tx.exec(pqxx::prepped{"Concat4Numbers"}, pqxx::params{tx, 1, params, 6})
      .one_row()};
  PQXX_CHECK_EQUAL(rw1396.front().as<int>(), 1396);
}


void test_optional(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  cx.prepare("EchoNum", "SELECT $1::int");
  pqxx::row rw{tx.exec(
                   pqxx::prepped{"EchoNum"},
                   pqxx::params{tx, std::optional<int>{std::in_place, 10}})
                 .one_row()};
  PQXX_CHECK_EQUAL(rw.front().as<int>(), 10);

  rw =
    tx.exec(pqxx::prepped{"EchoNum"}, pqxx::params{tx, std::optional<int>{}})
      .one_row();
  PQXX_CHECK(rw.front().is_null());
}


void test_prepared_statements(pqxx::test::context &tctx)
{
  test_registration_and_invocation(tctx);
  test_basic_args(tctx);
  test_multiple_params(tctx);
  test_nulls(tctx);
  test_strings(tctx);
  test_binary(tctx);
  test_params(tctx);

  test_optional(tctx);
}


void test_placeholders_generates_names(pqxx::test::context &)
{
  using pqxx::operator""_zv;
  pqxx::placeholders name;
  PQXX_CHECK_EQUAL(name.view(), "$1"_zv);
  PQXX_CHECK_EQUAL(name.view(), "$1"sv);
  PQXX_CHECK_EQUAL(name.get(), "$1");

  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$2"_zv);

  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$3"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$4"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$5"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$6"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$7"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$8"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$9"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$10"_zv);
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$11"_zv);

  while (name.count() < 999) name.next();
  PQXX_CHECK_EQUAL(name.view(), "$999"_zv, "Incorrect placeholders 999.");
  name.next();
  PQXX_CHECK_EQUAL(name.view(), "$1000"_zv);
}


void test_wrong_number_of_params(pqxx::test::context &)
{
  {
    pqxx::connection conn1;
    pqxx::transaction tx1{conn1};
    conn1.prepare("broken1", "SELECT $1::int + $2::int");
    PQXX_CHECK_THROWS(
      tx1.exec(pqxx::prepped{"broken1"}, pqxx::params{tx1, 10}),
      pqxx::protocol_violation);
  }

  {
    pqxx::connection conn2;
    pqxx::transaction tx2{conn2};
    conn2.prepare("broken2", "SELECT $1::int + $2::int");
    PQXX_CHECK_THROWS(
      tx2.exec(pqxx::prepped{"broken2"}, {5, 4, 3}), pqxx::protocol_violation);
  }
}


void test_query_prepped(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  cx.prepare("hop", "SELECT x * 3 FROM generate_series(1, 2) AS x");
  std::vector<int> out;
  for (auto [i] : tx.query<int>(pqxx::prepped{"hop"})) out.push_back(i);
  PQXX_CHECK_EQUAL(std::size(out), 2u);
  PQXX_CHECK_EQUAL(out.at(0), 3);
  PQXX_CHECK_EQUAL(out.at(1), 6);
}


void test_query_value_prepped(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  cx.prepare("pick", "SELECT 92");
  PQXX_CHECK_EQUAL(tx.query_value<int>(pqxx::prepped{"pick"}), 92);
}


void test_for_query_prepped(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};
  cx.prepare("series", "SELECT * FROM generate_series(3, 4)");
  std::vector<int> out;
  tx.for_query(pqxx::prepped("series"), [&out](int x) { out.push_back(x); });
  PQXX_CHECK_EQUAL(std::size(out), 2u);
  PQXX_CHECK_EQUAL(out.at(0), 3);
  PQXX_CHECK_EQUAL(out.at(1), 4);
}


void test_prepped_query_does_not_need_terminating_zero(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::transaction tx{cx};

  // Create name & definition without terminating zeroes.
  std::string_view const name_buf{"xblah123x'><;;"};
  auto const name{name_buf.substr(1, 4)};
  PQXX_CHECK_EQUAL(name, "blah");

  std::string_view const def_buf{"xSELECT $1+1x<>;;"};
  auto const def{def_buf.substr(1, 11)};
  PQXX_CHECK_EQUAL(std::string(def), "SELECT $1+1");

  cx.prepare(name, def);

  auto const res{tx.exec(pqxx::prepped{name}, pqxx::params{6})};
  PQXX_CHECK_EQUAL(res.at(0).at(0).view(), "7");

  cx.unprepare(name);

  // It also works with std::string arguments, and with query_value().
  cx.prepare(std::string{name}, def);
  PQXX_CHECK_EQUAL(
    tx.query_value<int>(pqxx::prepped{name}, pqxx::params{89}), 90);
}


PQXX_REGISTER_TEST(test_prepared_statements);
PQXX_REGISTER_TEST(test_placeholders_generates_names);
PQXX_REGISTER_TEST(test_wrong_number_of_params);
PQXX_REGISTER_TEST(test_query_prepped);
PQXX_REGISTER_TEST(test_query_value_prepped);
PQXX_REGISTER_TEST(test_for_query_prepped);
PQXX_REGISTER_TEST(test_prepped_query_does_not_need_terminating_zero);
} // namespace
