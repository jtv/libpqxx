#include "../test_helpers.hxx"

namespace
{
void test_result_iteration()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  PQXX_CHECK(r.end() != r.begin(), "Broken begin/end.");
  PQXX_CHECK(r.rend() != r.rbegin(), "Broken rbegin/rend.");

  PQXX_CHECK(r.cbegin() == r.begin(), "Wrong cbegin.");
  PQXX_CHECK(r.cend() == r.end(), "Wrong cend.");
  PQXX_CHECK(r.crbegin() == r.rbegin(), "Wrong crbegin.");
  PQXX_CHECK(r.crend() == r.rend(), "Wrong crend.");

  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 1, "Unexpected front().");
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 3, "Unexpected back().");
}


void test_result_iter()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  pqxx::result r{tx.exec("SELECT generate_series(1, 3)")};

  int total{0};
  for (auto const [i] : r.iter<int>()) total += i;
  PQXX_CHECK_EQUAL(total, 6, "iter() loop did not get the right values.");
}


PQXX_REGISTER_TEST(test_result_iteration);
PQXX_REGISTER_TEST(test_result_iter);
} // namespace
