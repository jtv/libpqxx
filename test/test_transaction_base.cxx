#include <pqxx/stream_to>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_exec0(pqxx::transaction_base &tx)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::result const E{tx.exec0("SELECT * FROM pg_tables WHERE 0 = 1")};
  PQXX_CHECK(std::empty(E));

  PQXX_CHECK_THROWS(tx.exec0("SELECT 99"), pqxx::unexpected_rows);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_exec1(pqxx::transaction_base &tx)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::row const R{tx.exec1("SELECT 99")};
  PQXX_CHECK_EQUAL(std::size(R), 1);
  PQXX_CHECK_EQUAL(R.front().as<int>(), 99);

  PQXX_CHECK_THROWS(
    tx.exec1("SELECT * FROM pg_tables WHERE 0 = 1"), pqxx::unexpected_rows);
  PQXX_CHECK_THROWS(
    tx.exec1("SELECT * FROM generate_series(1, 2)"), pqxx::unexpected_rows);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_exec_n(pqxx::transaction_base &tx)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::result const R{tx.exec_n(3u, "SELECT * FROM generate_series(1, 3)")};
  PQXX_CHECK_EQUAL(std::size(R), 3);

  PQXX_CHECK_THROWS(
    tx.exec_n(2u, "SELECT * FROM generate_series(1, 3)"),
    pqxx::unexpected_rows);
  PQXX_CHECK_THROWS(
    tx.exec_n(4u, "SELECT * FROM generate_series(1, 3)"),
    pqxx::unexpected_rows);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_query_value(pqxx::connection &cx)
{
  pqxx::work tx{cx};

  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 84 / 2"), 42);
  PQXX_CHECK_THROWS(tx.query_value<int>("SAVEPOINT dummy"), pqxx::usage_error);
  PQXX_CHECK_THROWS(
    tx.query_value<int>("SELECT generate_series(1, 2)"),
    pqxx::unexpected_rows);
  PQXX_CHECK_THROWS(tx.query_value<int>("SELECT 1, 2"), pqxx::usage_error);
  PQXX_CHECK_THROWS(
    tx.query_value<int>("SELECT 3.141"), pqxx::conversion_error);

  // Now with parameters:
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT $1 + 1", {tx, 5}), 6);
}


void test_transaction_base(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::work tx{cx};
    test_exec_n(tx);
    test_exec0(tx);
    test_exec1(tx);
  }
  test_query_value(cx);
}


void test_transaction_query(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::vector<std::string> names;
  std::vector<int> salaries;

  for (auto const &[name, salary] : tx.query<std::string, int>(
         "SELECT 'name' || i, i * 1000 FROM generate_series(1, 5) AS i"))
  {
    names.emplace_back(name);
    salaries.emplace_back(salary);
  }

  PQXX_CHECK_EQUAL(std::size(names), 5u);
  PQXX_CHECK_EQUAL(std::size(salaries), 5u);
  PQXX_CHECK_EQUAL(names.at(0), "name1");
  PQXX_CHECK_EQUAL(names.at(4), "name5");
  PQXX_CHECK_EQUAL(salaries.at(0), 1'000);
  PQXX_CHECK_EQUAL(salaries.at(4), 5'000, );
}


void test_transaction_query_params(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  int outcome{-1};

  for (auto [value] : tx.query<int>("SELECT $1 * 2", {32}))
  {
    PQXX_CHECK_EQUAL(outcome, -1);
    outcome = value;
  }
  PQXX_CHECK_EQUAL(outcome, 64);

  outcome = -1;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  for (auto [value] :
       tx.query_n<int>(1, "SELECT * FROM generate_series(1, $1)", {1}))
  {
    PQXX_CHECK_EQUAL(outcome, -1);
    outcome = value;
  }
  PQXX_CHECK_EQUAL(outcome, 1);

  PQXX_CHECK_THROWS(
    tx.query_n<int>(2, "SELECT $1", {9}), pqxx::unexpected_rows);

  std::tuple<int> const res{tx.query1<int>("SELECT $1 / 3", {33})};
  auto [res_int] = res;
  PQXX_CHECK_EQUAL(res_int, 11);

  PQXX_CHECK_THROWS(
    std::ignore = tx.query1<int>("SELECT * from generate_series(1, $1)", {4}),
    pqxx::unexpected_rows);

  std::tuple<int, int> const res2{
    tx.query1<int, int>("SELECT $1, $2", {3, 6})};
  auto [res2_a, res2_b] = res2;
  PQXX_CHECK_EQUAL(res2_a, 3);
  PQXX_CHECK_EQUAL(res2_b, 6);

  std::optional<std::tuple<int>> const opt1{
    tx.query01<int>("SELECT 1 WHERE 1 = $1", {0})};
  PQXX_CHECK(not opt1);
  std::optional<std::tuple<int>> const opt2{
    tx.query01<int>("SELECT $1 - 10", {12})};
  PQXX_CHECK(opt2.has_value());
  auto const [opt2_val] = opt2.value_or(0);
  PQXX_CHECK_EQUAL(opt2_val, 2);
  auto const [opt3_a, opt3_b] = tx.query01<int, int>("SELECT $1, $2", {12, 99})
                                  .value_or(std::tuple<int, int>{});
  PQXX_CHECK_EQUAL(opt3_a, 12);
  PQXX_CHECK_EQUAL(opt3_b, 99);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_transaction_for_query(pqxx::test::context &)
{
  constexpr auto query{
    "SELECT i, concat('x', (2*i)::text) "
    "FROM generate_series(1, 3) AS i "
    "ORDER BY i"};
  pqxx::connection cx;
  pqxx::work tx{cx};
  std::string ints;
  std::string strings;
  tx.for_query(query, [&ints, &strings](int i, std::string const &s) {
    ints += pqxx::to_string(i) + " ";
    strings += s + " ";
  });
  PQXX_CHECK_EQUAL(ints, "1 2 3 ");
  PQXX_CHECK_EQUAL(strings, "x2 x4 x6 ");

  // And now with parameters...
  int x{0}, y{0};
  tx.for_query(
    "SELECT $1, $2",
    [&x, &y](int xout, int yout) {
      PQXX_CHECK_EQUAL(x, 0);
      PQXX_CHECK_EQUAL(y, 0);
      x = xout;
      y = yout;
    },
    {42, 67});
  PQXX_CHECK_EQUAL(x, 42);
  PQXX_CHECK_EQUAL(y, 67);
}


void test_transaction_for_stream(pqxx::test::context &)
{
  constexpr auto query{
    "SELECT i, concat('x', (2*i)::text) "
    "FROM generate_series(1, 3) AS i "
    "ORDER BY i"};
  pqxx::connection cx;
  pqxx::work tx{cx};
  std::string ints;
  std::string strings;
  tx.for_stream(query, [&ints, &strings](int i, std::string const &s) {
    ints += pqxx::to_string(i) + " ";
    strings += s + " ";
  });
  PQXX_CHECK_EQUAL(ints, "1 2 3 ");
  PQXX_CHECK_EQUAL(strings, "x2 x4 x6 ");
}


void test_transaction_query01(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::optional<std::tuple<int>> o{
    tx.query01<int>("SELECT * FROM generate_series(1, 1) AS i WHERE i = 5")};
  PQXX_CHECK(not o.has_value());
  o = tx.query01<int>("SELECT * FROM generate_series(8, 8)");
  PQXX_CHECK(o.has_value());

  // The "if" is redundant (see the check above).  But gcc 11 complains when
  // enabling maintainer mode but not audit mode: something inside the optional
  // "may be used uninitialized."
  if (o.has_value())
    PQXX_CHECK_EQUAL(std::get<0>(*o), 8);
  PQXX_CHECK_THROWS(
    o = tx.query01<int>("SELECT * FROM generate_series(1, 2)"),
    pqxx::unexpected_rows);
  PQXX_CHECK_THROWS(o = tx.query01<int>("SELECT 1, 2"), pqxx::usage_error);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_transaction_query1(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  [[maybe_unused]] std::tuple<int> x;
  PQXX_CHECK_THROWS(
    x = tx.query1<int>("SELECT * FROM generate_series(1, 1) AS i WHERE i = 5"),
    pqxx::unexpected_rows);
  std::optional<std::tuple<int>> const o{
    tx.query1<int>("SELECT * FROM generate_series(8, 8)")};
  PQXX_CHECK(o.has_value());
  PQXX_CHECK_EQUAL(std::get<0>(*o), 8);
  PQXX_CHECK_THROWS(
    x = tx.query1<int>("SELECT * FROM generate_series(1, 2)"),
    pqxx::unexpected_rows);
  PQXX_CHECK_THROWS(x = tx.query1<int>("SELECT 1, 2"), pqxx::usage_error);
}


void test_transaction_query_n(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

#include "pqxx/internal/ignore-deprecated-pre.hxx"
  PQXX_CHECK_THROWS(
    std::ignore = tx.query_n<int>(5, "SELECT generate_series(1, 3)"),
    pqxx::unexpected_rows);
  PQXX_CHECK_THROWS(
    std::ignore = tx.query_n<int>(5, "SELECT generate_series(1, 10)"),
    pqxx::unexpected_rows);

  std::vector<int> v;
  for (auto [n] : tx.query_n<int>(3, "SELECT generate_series(7, 9)"))
    v.push_back(n);
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_EQUAL(std::size(v), 3u);
  PQXX_CHECK_EQUAL(v.at(0), 7);
  PQXX_CHECK_EQUAL(v.at(2), 9);
}


PQXX_REGISTER_TEST(test_transaction_base);
PQXX_REGISTER_TEST(test_transaction_query);
PQXX_REGISTER_TEST(test_transaction_query_params);
PQXX_REGISTER_TEST(test_transaction_for_query);
PQXX_REGISTER_TEST(test_transaction_for_stream);
PQXX_REGISTER_TEST(test_transaction_query01);
PQXX_REGISTER_TEST(test_transaction_query1);
PQXX_REGISTER_TEST(test_transaction_query_n);
} // namespace
