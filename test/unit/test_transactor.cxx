#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void test_transactor_newstyle_executes_simple_query()
{
  connection conn;
  const auto r = pqxx::perform(
    [&conn]{return work{conn}.exec("SELECT generate_series(1, 4)");});

  PQXX_CHECK_EQUAL(r.size(), 4u, "Unexpected result size.");
  PQXX_CHECK_EQUAL(r.columns(), 1u, "Unexpected number of columns.");
  PQXX_CHECK_EQUAL(r[0][0].as<int>(), 1, "Unexpected first row.");
  PQXX_CHECK_EQUAL(r[3][0].as<int>(), 4, "Unexpected last row.");
}


void test_transactor_newstyle_can_return_void()
{
  bool done = false;
  perform([&done]{ done = true; });
  PQXX_CHECK(done, "Callback was not executed.");
}


void test_transactor_newstyle_completes_upon_success()
{
  int attempts = 0;
  perform([&attempts](){ attempts++; });
  PQXX_CHECK_EQUAL(attempts, 1, "Successful transactor didn't run 1 time.");
}


void test_transactor_newstyle_retries_broken_connection()
{
  int counter = 0;
  const auto &callback = [&counter]{
    ++counter;
    if (counter == 1) throw pqxx::broken_connection();
    return counter;
    };

  const int result = pqxx::perform(callback);
  PQXX_CHECK_EQUAL(result, 2, "Transactor run returned wrong result.");
  PQXX_CHECK_EQUAL(counter, result, "Number of retries does not match.");
}


void test_transactor_newstyle_retries_rollback()
{
  int counter = 0;
  const auto &callback = [&counter]{
    ++counter;
    if (counter == 1) throw pqxx::transaction_rollback("Simulated error");
    return counter;
    };

  const int result = pqxx::perform(callback);
  PQXX_CHECK_EQUAL(result, 2, "Transactor run returned wrong result.");
  PQXX_CHECK_EQUAL(counter, result, "Number of retries does not match.");
}


void test_transactor_newstyle_does_not_retry_in_doubt_error()
{
  int counter = 0;
  const auto &callback = [&counter]{
    ++counter;
    throw pqxx::in_doubt_error("Simulated error");
    };

  PQXX_CHECK_THROWS(
	pqxx::perform(callback),
	in_doubt_error,
	"Transactor did not propagate in_doubt_error.");
  PQXX_CHECK_EQUAL(counter, 1, "Transactor retried after in_doubt_error.");
}


void test_transactor_newstyle_does_not_retry_other_error()
{
  int counter = 0;
  const auto &callback = [&counter]{
    ++counter;
    throw std::runtime_error("Simulated error");
    };

  PQXX_CHECK_THROWS(
	pqxx::perform(callback),
	std::runtime_error,
	"Transactor did not propagate std exception.");
  PQXX_CHECK_EQUAL(counter, 1, "Transactor retried after std exception.");
}


void test_transactor_newstyle_repeats_up_to_given_number_of_attempts()
{
  const int attempts = 5;
  int counter = 0;
  const auto &callback = [&counter]{
    ++counter;
    throw pqxx::transaction_rollback("Simulated error");
    };

  PQXX_CHECK_THROWS(
	pqxx::perform(callback, attempts),
        pqxx::transaction_rollback,
        "Not propagating original exception.");
  PQXX_CHECK_EQUAL(counter, attempts, "Number of retries does not match.");
}


void test_transactor()
{
  test_transactor_newstyle_executes_simple_query();
  test_transactor_newstyle_can_return_void();
  test_transactor_newstyle_completes_upon_success();
  test_transactor_newstyle_retries_broken_connection();
  test_transactor_newstyle_retries_rollback();
  test_transactor_newstyle_does_not_retry_in_doubt_error();
  test_transactor_newstyle_does_not_retry_other_error();
  test_transactor_newstyle_repeats_up_to_given_number_of_attempts();
}


PQXX_REGISTER_TEST(test_transactor);
} // namespace
