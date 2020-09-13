#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_result_iteration()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  PQXX_CHECK(std::end(r) != std::begin(r), "Broken begin/end.");
  PQXX_CHECK(std::rend(r) != std::rbegin(r), "Broken rbegin/rend.");

  PQXX_CHECK(std::cbegin(r) == std::begin(r), "Wrong cbegin.");
  PQXX_CHECK(std::cend(r) == std::end(r), "Wrong cend.");
  PQXX_CHECK(std::crbegin(r) == std::rbegin(r), "Wrong crbegin.");
  PQXX_CHECK(std::crend(r) == std::rend(r), "Wrong crend.");

  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 1, "Unexpected front().");
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 3, "Unexpected back().");
}


void test_result_iter()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  int total{0};
  for (auto const &[i] : r.iter<int>()) total += i;
  PQXX_CHECK_EQUAL(total, 6, "iter() loop did not get the right values.");
}


void test_result_iterator_swap()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  auto head{std::begin(r)}, next{head + 1};
  head.swap(next);
  PQXX_CHECK_EQUAL(head[0].as<int>(), 2, "Result iterator swap is wrong.");
  PQXX_CHECK_EQUAL(next[0].as<int>(), 1, "Result iterator swap is crazy.");

  auto tail{std::rbegin(r)}, prev{tail + 1};
  tail.swap(prev);
  PQXX_CHECK_EQUAL(tail[0].as<int>(), 2, "Reverse iterator swap is wrong.");
  PQXX_CHECK_EQUAL(prev[0].as<int>(), 3, "Reverse iterator swap is crazy.");
}


void test_result_iterator_assignment()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  pqxx::result::const_iterator fwd;
  pqxx::result::const_reverse_iterator rev;

  fwd = std::begin(r);
  PQXX_CHECK_EQUAL(
    fwd[0].as<int>(), std::begin(r)[0].as<int>(),
    "Result iterator assignment is wrong.");

  rev = std::rbegin(r);
  PQXX_CHECK_EQUAL(
    rev[0].as<int>(), std::rbegin(r)[0].as<int>(),
    "Reverse iterator assignment is wrong.");
}


PQXX_REGISTER_TEST(test_result_iteration);
PQXX_REGISTER_TEST(test_result_iter);
PQXX_REGISTER_TEST(test_result_iterator_swap);
PQXX_REGISTER_TEST(test_result_iterator_assignment);
} // namespace
