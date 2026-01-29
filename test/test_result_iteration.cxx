#include <iterator>

#include <pqxx/stream_to>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_result_iteration(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};
  static_assert(std::random_access_iterator<decltype(r.begin())>);
  static_assert(std::random_access_iterator<decltype(r.begin()->begin())>);

  PQXX_CHECK(std::end(r) != std::begin(r));
  PQXX_CHECK(std::rend(r) != std::rbegin(r));

  PQXX_CHECK(std::cbegin(r) == std::begin(r));
  PQXX_CHECK(std::cend(r) == std::end(r));
  PQXX_CHECK(std::crbegin(r) == std::rbegin(r));
  PQXX_CHECK(std::crend(r) == std::rend(r));

  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 1);
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 3);

  PQXX_CHECK((pqxx::result::const_iterator{r, 1} != r.begin()));
  PQXX_CHECK((pqxx::result::const_iterator{r, 1} == r.begin() + 1));
}


void test_result_iter(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::result const r{tx.exec("SELECT generate_series(1, 3)")};

  int total{0};
  for (auto const &[i] : r.iter<int>()) total += i;
  PQXX_CHECK_EQUAL(total, 6);

  pqxx::result::const_iterator c{r.begin()};
  auto const oldit{c++};
  PQXX_CHECK(oldit == r.begin());
  PQXX_CHECK(c == r.begin() + 1);
  auto const newit{++c};
  PQXX_CHECK(newit == c);
  PQXX_CHECK(newit == r.begin() + 2);

  auto const backit{--c};
  PQXX_CHECK(backit == r.begin() + 1);
  PQXX_CHECK(c-- == r.begin() + 1);
  PQXX_CHECK(c == r.begin());
}


void test_result_iterator_swap(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  auto head{std::begin(r)}, next{head + 1};
  head.swap(next);
  PQXX_CHECK_EQUAL((*head)[0].as<int>(), 2);
  PQXX_CHECK_EQUAL((*next)[0].as<int>(), 1);

  auto tail{std::rbegin(r)}, prev{tail + 1};
  tail.swap(prev);
  PQXX_CHECK_EQUAL((*tail)[0].as<int>(), 2);
  PQXX_CHECK_EQUAL((*prev)[0].as<int>(), 3);
}


void test_result_iterator_assignment(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  pqxx::result::const_iterator fwd;
  pqxx::result::const_reverse_iterator rev;

  fwd = std::begin(r);
  PQXX_CHECK_EQUAL((*fwd)[0].as<int>(), (*std::begin(r))[0].as<int>());

  rev = std::rbegin(r);
  PQXX_CHECK_EQUAL((*rev)[0].as<int>(), (*std::rbegin(r))[0].as<int>());

  pqxx::result::const_reverse_iterator const rev2{fwd};
  PQXX_CHECK(rev2 == std::rend(r));

  pqxx::result::const_iterator pos;
  pos = {r, 1u};
  PQXX_CHECK_EQUAL(pos->at(0).view(), "2");
  ++pos;
  PQXX_CHECK_EQUAL(pos->at(0).view(), "3");
  --pos;
  PQXX_CHECK_EQUAL(pos->at(0).view(), "2");
  pos += 2;
  PQXX_CHECK(pos == r.end());
}


void check_employee(std::string const &name, int salary)
{
  PQXX_CHECK(name == "x" or name == "y" or name == "z");
  PQXX_CHECK(salary == 1000 or salary == 1200 or salary == 1500);
}


void test_result_for_each(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE employee(name varchar, salary int)").no_rows();
  auto fill{pqxx::stream_to::table(tx, {"employee"}, {"name", "salary"})};
  fill.write_values("x", 1000);
  fill.write_values("y", 1200);
  fill.write_values("z", 1500);
  fill.complete();

  auto const res{tx.exec("SELECT name, salary FROM employee ORDER BY name")};

  // Use for_each with a function.
  res.for_each(check_employee);

  // Use for_each with a simple lambda.
  res.for_each(
    [](std::string const &name, int salary) { check_employee(name, salary); });

  // Use for_each with a lambda closure.
  std::string names{};
  int total{0};

  res.for_each([&names, &total](std::string const &name, int salary) {
    names.append(name);
    total += salary;
  });
  PQXX_CHECK_EQUAL(names, "xyz");
  PQXX_CHECK_EQUAL(total, 1000 + 1200 + 1500);

  // In addition to regular conversions, you can receive arguments as
  // string_view, or as references.
  names.clear();
  total = 0;
  res.for_each([&names, &total](std::string_view &&name, int const &salary) {
    names.append(name);
    total += salary;
  });
  PQXX_CHECK_EQUAL(names, "xyz");
  PQXX_CHECK_EQUAL(total, 1000 + 1200 + 1500);
}


PQXX_REGISTER_TEST(test_result_iteration);
PQXX_REGISTER_TEST(test_result_iter);
PQXX_REGISTER_TEST(test_result_iterator_swap);
PQXX_REGISTER_TEST(test_result_iterator_assignment);
PQXX_REGISTER_TEST(test_result_for_each);
} // namespace
