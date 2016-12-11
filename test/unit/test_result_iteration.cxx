#include <test_helpers.hxx>

namespace
{
void test_result_iteration(pqxx::transaction_base &trans)
{
  pqxx::test::prepare_series(trans, 1, 3);
  pqxx::result r = trans.exec(pqxx::test::select_series(trans.conn(), 1, 3));

  PQXX_CHECK(r.end() != r.begin(), "Broken begin/end.");
  PQXX_CHECK(r.rend() != r.rbegin(), "Broken rbegin/rend.");

  PQXX_CHECK(r.cbegin() == r.begin(), "Wrong cbegin.");
  PQXX_CHECK(r.cend() == r.end(), "Wrong cend.");
  PQXX_CHECK(r.crbegin() == r.rbegin(), "Wrong crbegin.");
  PQXX_CHECK(r.crend() == r.rend(), "Wrong crend.");

  PQXX_CHECK_EQUAL(r.front().front().as<int>(), 1, "Unexpected front().");
  PQXX_CHECK_EQUAL(r.back().front().as<int>(), 3, "Unexpected back().");
}
} // namespace

PQXX_REGISTER_TEST(test_result_iteration)
