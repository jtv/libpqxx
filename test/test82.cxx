#include <pqxx/nontransaction>

#include "helpers.hxx"


// Test program for libpqxx.  Read and print table using row iterators.
namespace
{
void test_082(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::nontransaction tx{cx};

  pqxx::test::create_pqxxevents(tx);
  std::string const Table{"pqxxevents"};
  pqxx::result const R{tx.exec("SELECT * FROM " + Table)};

  PQXX_CHECK(not std::empty(R));

  std::string const nullstr("[null]");

  for (auto const &r : R)
  {
    pqxx::row::const_iterator f2(r[0]);
    for (auto const &f : r)
    {
      PQXX_CHECK_EQUAL((*f2).as(nullstr), f.as(nullstr));
      ++f2;
    }

    PQXX_CHECK(
      std::begin(r) + pqxx::row::difference_type(std::size(r)) == std::end(r));
    PQXX_CHECK(
      pqxx::row::difference_type(std::size(r)) + std::begin(r) == std::end(r));
    PQXX_CHECK_EQUAL(std::begin(r)->column_number(), 0);

    pqxx::row::const_iterator f3(r[std::size(r)]);

    PQXX_CHECK(f3 == std::end(r));

    PQXX_CHECK(f3 > std::begin(r));

    PQXX_CHECK(f3 >= std::end(r) and std::begin(r) < f3);

    PQXX_CHECK(f3 > std::begin(r));

    pqxx::row::const_iterator f4{r, std::size(r)};
    PQXX_CHECK(f4 == f3);

    --f3;
    f4 -= 1;

    PQXX_CHECK(f3 < std::end(r));
    PQXX_CHECK(f3 >= std::begin(r));
    PQXX_CHECK(f3 == std::end(r) - 1);
    PQXX_CHECK_EQUAL(std::end(r) - f3, 1);

    PQXX_CHECK(f4 == f3);
    f4 += 1;
    PQXX_CHECK(f4 == std::end(r));

    for (auto fr = std::rbegin(r); fr != std::rend(r); ++fr, --f3)
      PQXX_CHECK_EQUAL(fr->as<std::string>(), f3->as<std::string>());
  }

  // Thorough test for row::const_reverse_iterator
  pqxx::row::const_reverse_iterator const ri1(std::rbegin(R.front()));
  pqxx::row::const_reverse_iterator ri2(ri1), ri3(std::end(R.front()));
  ri2 = std::rbegin(R.front());

  PQXX_CHECK(ri1 == ri2);

  PQXX_CHECK(ri2 == ri3);
  PQXX_CHECK_EQUAL(ri2 - ri3, 0);

  PQXX_CHECK(pqxx::row::const_reverse_iterator(ri1.base()) == ri1);

  PQXX_CHECK(ri2 == ri3 + 0);
  PQXX_CHECK(ri2 == ri3 - 0);

  PQXX_CHECK(not(ri3 < ri2));
  PQXX_CHECK(ri2 <= ri3);
  PQXX_CHECK(ri3++ == ri2);

  PQXX_CHECK_EQUAL(ri3 - ri2, 1);
  PQXX_CHECK(ri3 > ri2);
  PQXX_CHECK(ri3 >= ri2);
  PQXX_CHECK(ri2 < ri3);
  PQXX_CHECK(ri2 <= ri3);
  PQXX_CHECK(ri3 == ri2 + 1);
  PQXX_CHECK(ri2 == ri3 - 1, );

  PQXX_CHECK(ri3 == ++ri2);

  PQXX_CHECK(ri3 >= ri2);
  PQXX_CHECK(ri3 >= ri2);
  PQXX_CHECK_EQUAL(
    ri3.base()->as<std::string>(), R.front().back().as<std::string>());
  PQXX_CHECK(ri1->c_str()[0] == (*ri1).c_str()[0]);
  PQXX_CHECK(ri2-- == ri3);
  PQXX_CHECK(ri2 == --ri3);
  PQXX_CHECK(ri2 == std::rbegin(R.front()));

  ri2 += 1;
  ri3 -= -1;

  PQXX_CHECK(ri2 != std::rbegin(R.front()));
  PQXX_CHECK(ri2 != std::rbegin(R.front()));
  PQXX_CHECK(ri3 == ri2);

  ri2 -= 1;
  PQXX_CHECK(ri2 == std::rbegin(R.front()));
}
} // namespace


PQXX_REGISTER_TEST(test_082);
