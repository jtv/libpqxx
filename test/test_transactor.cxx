#include <pqxx/transaction>
#include <pqxx/transactor>

#include "helpers.hxx"

namespace
{
void test_transactor_newstyle_executes_simple_query(pqxx::test::context &)
{
  pqxx::connection cx;
  auto const r{pqxx::perform(
    [&cx] { return pqxx::work{cx}.exec("SELECT generate_series(1, 4)"); })};

  PQXX_CHECK_EQUAL(std::size(r), 4);
  PQXX_CHECK_EQUAL(r.columns(), 1);
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 1);
  PQXX_CHECK_EQUAL(r[3][0].as<int>(), 4);
}


void test_transactor_newstyle_can_return_void(pqxx::test::context &)
{
  bool done{false};
  pqxx::perform([&done]() noexcept { done = true; });
  PQXX_CHECK(done);
}


void test_transactor_newstyle_completes_upon_success(pqxx::test::context &)
{
  int attempts{0};
  pqxx::perform([&attempts]() noexcept { attempts++; });
  PQXX_CHECK_EQUAL(attempts, 1);
}


void test_transactor_newstyle_retries_broken_connection(pqxx::test::context &)
{
  int counter{0};
  auto const &callback{[&counter] {
    ++counter;
    if (counter == 1)
      throw pqxx::broken_connection{};
    return counter;
  }};

  int const result{pqxx::perform(callback)};
  PQXX_CHECK_EQUAL(result, 2);
  PQXX_CHECK_EQUAL(counter, result);
}


void test_transactor_newstyle_retries_rollback(pqxx::test::context &)
{
  int counter{0};
  auto const &callback{[&counter] {
    ++counter;
    if (counter == 1)
      throw pqxx::transaction_rollback("Simulated error");
    return counter;
  }};

  int const result{pqxx::perform(callback)};
  PQXX_CHECK_EQUAL(result, 2);
  PQXX_CHECK_EQUAL(counter, result);
}


void test_transactor_newstyle_does_not_retry_in_doubt_error(
  pqxx::test::context &)
{
  int counter{0};
  auto const &callback{[&counter] {
    ++counter;
    throw pqxx::in_doubt_error("Simulated error");
  }};

  PQXX_CHECK_THROWS(pqxx::perform(callback), pqxx::in_doubt_error);
  PQXX_CHECK_EQUAL(counter, 1, "Transactor retried after in_doubt_error.");
}


void test_transactor_newstyle_does_not_retry_other_error(pqxx::test::context &)
{
  int counter{0};
  auto const &callback{[&counter] {
    ++counter;
    throw std::runtime_error("Simulated error");
  }};

  PQXX_CHECK_THROWS(pqxx::perform(callback), std::runtime_error);
  PQXX_CHECK_EQUAL(counter, 1);
}


void test_transactor_newstyle_repeats_up_to_given_number_of_attempts(
  pqxx::test::context &)
{
  int const attempts{5};
  int counter{0};
  auto const &callback{[&counter] {
    ++counter;
    throw pqxx::transaction_rollback("Simulated error");
  }};

  PQXX_CHECK_THROWS(
    pqxx::perform(callback, attempts), pqxx::transaction_rollback);
  PQXX_CHECK_EQUAL(counter, attempts);
}


void test_transactor(pqxx::test::context &tctx)
{
  test_transactor_newstyle_executes_simple_query(tctx);
  test_transactor_newstyle_can_return_void(tctx);
  test_transactor_newstyle_completes_upon_success(tctx);
  test_transactor_newstyle_retries_broken_connection(tctx);
  test_transactor_newstyle_retries_rollback(tctx);
  test_transactor_newstyle_does_not_retry_in_doubt_error(tctx);
  test_transactor_newstyle_does_not_retry_other_error(tctx);
  test_transactor_newstyle_repeats_up_to_given_number_of_attempts(tctx);
}


PQXX_REGISTER_TEST(test_transactor);
} // namespace
