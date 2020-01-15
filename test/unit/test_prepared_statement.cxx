#include <cassert>
#include <iostream>
#include <iterator>
#include <list>

#include "../test_helpers.hxx"

// Test program for libpqxx.  Define and use prepared statements.

#define COMPARE_RESULTS(name, lhs, rhs)                                       \
  PQXX_CHECK_EQUAL(                                                           \
    rhs, lhs,                                                                 \
    "Executing " name " as prepared statement yields different results.");

namespace
{
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
  ptrdiff_t i = distance(patbegin, patend);
  for (ITER arg = patend; i > 0; --i)
  {
    --arg;
    const std::string marker = "$" + pqxx::to_string(i),
                      var = stringize(t, *arg);
    const std::string::size_type msz = marker.size();
    while (q.find(marker) != std::string::npos)
      q.replace(q.find(marker), msz, var);
  }
  return q;
}


template<typename CNTNR>
std::string
subst(pqxx::transaction_base &t, std::string q, const CNTNR &patterns)
{
  return subst(t, q, patterns.begin(), patterns.end());
}


void test_registration_and_invocation()
{
  constexpr auto count_to_5 = "SELECT * FROM generate_series(1, 5)";

  pqxx::connection c;
  pqxx::work tx1{c};

  // Prepare a simple statement.
  tx1.conn().prepare("CountToFive", count_to_5);

  // The statement returns exactly what you'd expect.
  COMPARE_RESULTS(
    "CountToFive", tx1.exec_prepared("CountToFive"), tx1.exec(count_to_5));

  // Re-preparing it is an error.
  PQXX_CHECK_THROWS(
    tx1.conn().prepare("CountToFive", count_to_5), pqxx::sql_error,
    "Did not report re-definition of prepared statement.");

  tx1.abort();
  pqxx::work tx2{c};

  // Executing a nonexistent prepared statement is also an error.
  PQXX_CHECK_THROWS(
    tx2.exec_prepared("NonexistentStatement"), pqxx::sql_error,
    "Did not report invocation of nonexistent prepared statement.");
}


void test_basic_args()
{
  pqxx::connection c;
  c.prepare("EchoNum", "SELECT $1::int");
  pqxx::work tx{c};
  auto r = tx.exec_prepared("EchoNum", 7);
  PQXX_CHECK_EQUAL(r.size(), 1, "Did not get 1 row from prepared statement.");
  PQXX_CHECK_EQUAL(r.front().size(), 1, "Did not get exactly one column.");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 7, "Got wrong result.");

  auto rw = tx.exec_prepared1("EchoNum", 8);
  PQXX_CHECK_EQUAL(rw.size(), 1, "Did not get 1 column from exec_prepared1.");
  PQXX_CHECK_EQUAL(rw[0].as<int>(), 8, "Got wrong result.");
}


void test_multiple_params()
{
  pqxx::connection c;
  c.prepare("CountSeries", "SELECT * FROM generate_series($1::int, $2::int)");
  pqxx::work tx{c};
  auto r = tx.exec_prepared_n(4, "CountSeries", 7, 10);
  PQXX_CHECK_EQUAL(r.size(), 4, "Wrong number of rows, but no error raised.");
  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 7, "Wrong $1.");
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 10, "Wrong $2.");

  c.prepare("Reversed", "SELECT * FROM generate_series($2::int, $1::int)");
  r = tx.exec_prepared_n(3, "Reversed", 8, 6);
  PQXX_CHECK_EQUAL(
    r.front().front().as<int>(), 6, "Did parameters get reordered?");
  PQXX_CHECK_EQUAL(
    r.back().front().as<int>(), 8, "$2 did not come through properly.");
}


void test_nulls()
{
  pqxx::connection c;
  pqxx::work tx{c};
  c.prepare("EchoStr", "SELECT $1::varchar");
  auto rw = tx.exec_prepared1("EchoStr", nullptr);
  PQXX_CHECK(rw.front().is_null(), "nullptr did not translate to null.");

  const char *n = nullptr;
  rw = tx.exec_prepared1("EchoStr", n);
  PQXX_CHECK(rw.front().is_null(), "Null pointer did not translate to null.");
}


void test_strings()
{
  pqxx::connection c;
  pqxx::work tx{c};
  c.prepare("EchoStr", "SELECT $1::varchar");
  auto rw = tx.exec_prepared1("EchoStr", "foo");
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), "foo", "Wrong string result.");

  const char nasty_string[] = R"--('\"\)--";
  rw = tx.exec_prepared1("EchoStr", nasty_string);
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), std::string(nasty_string),
    "Prepared statement did not quote/escape correctly.");

  rw = tx.exec_prepared1("EchoStr", std::string{nasty_string});
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), std::string(nasty_string),
    "Quoting/escaping went wrong in std::string.");

  char nonconst[] = "non-const C string";
  rw = tx.exec_prepared1("EchoStr", nonconst);
  PQXX_CHECK_EQUAL(
    rw.front().as<std::string>(), std::string(nonconst),
    "Non-const C string passed incorrectly.");
}


void test_binary()
{
  pqxx::connection c;
  pqxx::work tx{c};
  c.prepare("EchoBin", "SELECT $1::bytea");
  const char raw_bytes[] = "Binary\0bytes'\"with\tweird\xff bytes";
  const std::string input{raw_bytes, sizeof(raw_bytes)};
  const pqxx::binarystring bin{input};

  auto rw = tx.exec_prepared1("EchoBin", bin);
  PQXX_CHECK_EQUAL(
    pqxx::binarystring(rw.front()).str(), input,
    "Binary string came out damaged.");
}


void test_dynamic_params()
{
  pqxx::connection c;
  pqxx::work tx{c};
  c.prepare("Concat2Numbers", "SELECT 10 * $1 + $2");
  std::vector<int> values{3, 9};
  const auto params = pqxx::prepare::make_dynamic_params(values);
  const auto rw39 = tx.exec_prepared1("Concat2Numbers", params);
  PQXX_CHECK_EQUAL(
    rw39.front().as<int>(), 39,
    "Dynamic prepared-statement parameters went wrong.");

  c.prepare("Concat4Numbers", "SELECT 1000*$1 + 100*$2 + 10*$3 + $4");
  const auto rw1396 = tx.exec_prepared1("Concat4Numbers", 1, params, 6);
  PQXX_CHECK_EQUAL(
    rw1396.front().as<int>(), 1396,
    "Dynamic params did not interleave with static ones properly.");

  const auto doubled = tx.exec_prepared1(
    "Concat2Numbers", pqxx::prepare::make_dynamic_params(
                        values, [](const int &i) { return 2 * i; }));
  PQXX_CHECK_EQUAL(
    doubled.at(0).as<int>(), 2 * 39,
    "Dynamic prepared-statement parameters went wrong.");
}


void test_optional()
{
  pqxx::connection c;
  pqxx::work tx{c};
  c.prepare("EchoNum", "SELECT $1::int");
  pqxx::row rw =
    tx.exec_prepared1("EchoNum", std::optional<int>{std::in_place, 10});
  PQXX_CHECK_EQUAL(
    rw.front().as<int>(), 10,
    "optional (with value) did not return the right value.");

  rw = tx.exec_prepared1("EchoNum", std::optional<int>{});
  PQXX_CHECK(
    rw.front().is_null(), "optional without value did not come out as null.");
}


void test_prepared_statements()
{
  test_registration_and_invocation();
  test_basic_args();
  test_multiple_params();
  test_nulls();
  test_strings();
  test_binary();
  test_dynamic_params();

  test_optional();
}


PQXX_REGISTER_TEST(test_prepared_statements);
} // namespace
