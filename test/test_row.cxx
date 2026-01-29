#include <iterator>

#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_row(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::row r{tx.exec("SELECT 1 AS one, 2 AS two, 3 AS three").one_row()};
  static_assert(std::forward_iterator<decltype(r.begin())>);
  PQXX_CHECK_EQUAL(std::size(r), 3);
  PQXX_CHECK_EQUAL(r.at(0).as<int>(), 1);
  PQXX_CHECK(std::begin(r) != std::end(r));
  PQXX_CHECK(std::begin(r) < std::end(r));
  PQXX_CHECK(std::cbegin(r) == std::begin(r));
  PQXX_CHECK(std::cend(r) == std::end(r));
  PQXX_CHECK(std::rbegin(r) != std::rend(r));
  PQXX_CHECK(std::crbegin(r) == std::rbegin(r));
  PQXX_CHECK(std::crend(r) == std::rend(r));
  PQXX_CHECK_EQUAL(r.front().as<int>(), 1);
  PQXX_CHECK_EQUAL(r.back().as<int>(), 3);

  PQXX_CHECK_THROWS(std::ignore = r.at(3), pqxx::range_error);
  PQXX_CHECK_EQUAL(r.at("two").view(), "2");
  PQXX_CHECK_THROWS(std::ignore = r.at("four"), pqxx::argument_error);
}


void test_row_iterator(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  pqxx::result const rows{tx.exec("SELECT 1, 2, 3")};
  // Very important to keep this in a variable.  We'll be creating an iterator
  // on it, and if we used a temporary here, it'd go out of scope, get
  // destroyed, and invalidate the accesses!
  auto row{rows[0]};
  auto i{std::begin(row)};
  PQXX_CHECK_EQUAL(i->as<int>(), 1);
  auto i2{i};
  PQXX_CHECK_EQUAL(i2->as<int>(), 1);
  i2++;
  PQXX_CHECK_EQUAL(i2->as<int>(), 2);
  pqxx::row::const_iterator i3;
  i3 = i2;
  PQXX_CHECK_EQUAL(i3->as<int>(), 2);

  auto r{std::rbegin(row)};
  PQXX_CHECK_EQUAL(r->as<int>(), 3);
  auto r2{r};
  PQXX_CHECK_EQUAL(r2->as<int>(), 3);
  r2++;
  PQXX_CHECK_EQUAL(r2->as<int>(), 2);
  pqxx::row::const_reverse_iterator r3;
  r3 = r2;
  PQXX_CHECK_EQUAL(i3->as<int>(), 2);
}


void test_row_as(pqxx::test::context &)
{
  using pqxx::operator""_zv;

  pqxx::connection cx;
  pqxx::work tx{cx};

  pqxx::row const r{tx.exec("SELECT 1, 2, 3").one_row()};
  auto [one, two, three]{r.as<int, float, pqxx::zview>()};
  static_assert(std::is_same_v<decltype(one), int>);
  static_assert(std::is_same_v<decltype(two), float>);
  static_assert(std::is_same_v<decltype(three), pqxx::zview>);
  PQXX_CHECK_EQUAL(one, 1);
  PQXX_CHECK_GREATER(two, 1.9);
  PQXX_CHECK_LESS(two, 2.1);
  PQXX_CHECK_EQUAL(three, "3"_zv);

  PQXX_CHECK_EQUAL(
    std::get<0>(tx.exec("SELECT 999").one_row().as<int>()), 999);
}


// In a random access iterator i, i[n] == *(i + n).
void test_row_iterator_array_index_offsets_iterator(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const row{tx.exec("SELECT 5, 4, 3, 2").one_row()};
  PQXX_CHECK_EQUAL(row.begin()[1].as<std::string>(), "4");
  PQXX_CHECK_EQUAL(row.rbegin()[1].as<std::string>(), "3");
}


void test_row_as_tuple(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  auto const r{tx.exec("SELECT 1, 'Alice'").one_row()};

  // A tuple type with matching number and types of columns.
  using correct_tuple_t = std::tuple<int, std::string>;
  // Tuple types with the wrong numbers of columns.
  using short_tuple_t = std::tuple<int>;
  using long_tuple_t = std::tuple<int, std::string, int>;

  PQXX_CHECK_EQUAL(r.size(), 2);
  correct_tuple_t t{r.as_tuple<correct_tuple_t>()};

  PQXX_CHECK_EQUAL(std::get<0>(t), 1);
  PQXX_CHECK_EQUAL(std::get<1>(t), "Alice");

  PQXX_CHECK_THROWS(r.as_tuple<short_tuple_t>(), pqxx::usage_error);
  PQXX_CHECK_THROWS(r.as_tuple<long_tuple_t>(), pqxx::usage_error);
}


void test_row_swap(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  pqxx::result const res{tx.exec("SELECT * FROM generate_series(1,3)")};
  pqxx::row r1{res.at(0)}, r3{res.at(2)};

  PQXX_CHECK_EQUAL(r1.at(0).view(), "1");
  PQXX_CHECK_EQUAL(r3.at(0).view(), "3");

#include "pqxx/internal/ignore-deprecated-pre.hxx"
  r1.swap(r3);
#include "pqxx/internal/ignore-deprecated-post.hxx"

  // The two rows' positions have switched places.
  PQXX_CHECK_EQUAL(r1.at(0).view(), "3");
  PQXX_CHECK_EQUAL(r3.at(0).view(), "1");

  // The original row remains unaffected.
  PQXX_CHECK_EQUAL(res.at(0).at(0).view(), "1");
  PQXX_CHECK_EQUAL(res.at(1).at(0).view(), "2");
  PQXX_CHECK_EQUAL(res.at(2).at(0).view(), "3");

#include "pqxx/internal/ignore-deprecated-pre.hxx"
  r1.swap(r3);
#include "pqxx/internal/ignore-deprecated-post.hxx"

  // Now they're back in their original positions.
  PQXX_CHECK_EQUAL(r1.at(0).view(), "1");
  PQXX_CHECK_EQUAL(r3.at(0).view(), "3");

  // It doesn't matter whether we a.swap(b) or b.swap(a).
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  r3.swap(r1);
#include "pqxx/internal/ignore-deprecated-post.hxx"

  PQXX_CHECK_EQUAL(r1.at(0).view(), "3");
  PQXX_CHECK_EQUAL(r3.at(0).view(), "1");
}


PQXX_REGISTER_TEST(test_row);
PQXX_REGISTER_TEST(test_row_iterator);
PQXX_REGISTER_TEST(test_row_as);
PQXX_REGISTER_TEST(test_row_iterator_array_index_offsets_iterator);
PQXX_REGISTER_TEST(test_row_as_tuple);
PQXX_REGISTER_TEST(test_row_swap);
} // namespace
