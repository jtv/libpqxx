#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_notice_handler_receives_notice()
{
  pqxx::connection cx;
  int notices{0};

  cx.set_notice_handler([&notices](pqxx::zview) noexcept { ++notices; });

  pqxx::work tx{cx};
  // Start a transaction while already in a transaction, to trigger a notice.
  tx.exec("BEGIN").no_rows();

  PQXX_CHECK_EQUAL(notices, 1, "Did not get expected single notice.");
}


void test_notice_handler_works_after_connection_closes()
{
  pqxx::result r;
  int notices{0};

  {
    pqxx::connection cx;
    cx.set_notice_handler([&notices](pqxx::zview) noexcept { ++notices; });
    pqxx::work tx{cx};
    r = tx.exec("SELECT 1");
  }

  PQXX_CHECK_EQUAL(notices, 0, "Got premature notice.");

  // Trigger a notice by asking libpq about a nonexistent column.
  PQXX_CHECK_THROWS_EXCEPTION(
    pqxx::ignore_unused(r.table_column(99)),
    "Expected an out-of-bounds table_column() to throw an error.");

  PQXX_CHECK_EQUAL(
    notices, 1, "Did not get expected single post-connection notice.");
}


void test_process_notice_calls_notice_handler()
{
  int calls{0};
  std::string received;
  const std::string msg{"Hello there\n"};

  pqxx::connection cx;
  cx.set_notice_handler([&calls, &received](auto x) noexcept {
    ++calls;
    received = x;
  });
  cx.process_notice(msg);

  PQXX_CHECK_EQUAL(calls, 1, "Expected exactly 1 call to notice handler.");
  PQXX_CHECK_EQUAL(received, msg, "Got wrong message.");
}


// Global counter so we can count calls to a global function.
int notice_handler_test_func_counter{0};

void notice_handler_test_func(pqxx::zview)
{
  ++notice_handler_test_func_counter;
}


void test_notice_handler_accepts_function()
{
  pqxx::connection cx;
  cx.set_notice_handler(notice_handler_test_func);
  cx.process_notice("Hello");
  PQXX_CHECK_EQUAL(notice_handler_test_func_counter, 1, "Expected 1 call.");
}


int notice_handler_test_lambda_counter{0};

void test_notice_handler_accepts_stateless_lambda()
{
  pqxx::connection cx;
  cx.set_notice_handler(
    [](pqxx::zview) noexcept { ++notice_handler_test_lambda_counter; });
  cx.process_notice("Hello");
  PQXX_CHECK_EQUAL(notice_handler_test_lambda_counter, 1, "Expected 1 call.");
}


struct notice_handler_test_functor
{
  int &count;
  std::string &received;

  notice_handler_test_functor(int &c, std::string &r) : count{c}, received{r}
  {}

  void operator()(pqxx::zview msg) noexcept
  {
    ++count;
    received = msg;
  }
};


void test_notice_handler_accepts_functor()
{
  std::string const hello{"Hello world"};

  // We're going to create a functor that stores its call count and message
  // her.  We can't store it inside the functor, because that gets passed by
  // value to the connection as a std::function.
  int count{0};
  std::string received;
  notice_handler_test_functor f(count, received);

  pqxx::connection cx;
  cx.set_notice_handler(f);
  cx.process_notice(hello);

  PQXX_CHECK_EQUAL(count, 1, "Expected 1 call.");
  PQXX_CHECK_EQUAL(received, hello, "Wrong message.");
}


void test_notice_handler_works_after_moving_connection()
{
  bool got_message{false};
  pqxx::connection cx;
  cx.set_notice_handler(
    [&got_message](pqxx::zview) noexcept { got_message = true; });
  auto cx2{std::move(cx)};
  pqxx::connection cx3;
  cx3 = std::move(cx2);
  pqxx::work tx{cx3};

  // Trigger a notice.  Just calling process_notice() isn't hard enough for a
  // good strong test, because that function bypasses the libpq logic for
  // receiving a notice.
  tx.exec("BEGIN").no_rows();

  PQXX_CHECK(got_message, "Did not receive notice after moving.");
}


PQXX_REGISTER_TEST(test_notice_handler_receives_notice);
PQXX_REGISTER_TEST(test_notice_handler_works_after_connection_closes);
PQXX_REGISTER_TEST(test_process_notice_calls_notice_handler);
PQXX_REGISTER_TEST(test_notice_handler_accepts_function);
PQXX_REGISTER_TEST(test_notice_handler_accepts_stateless_lambda);
PQXX_REGISTER_TEST(test_notice_handler_accepts_functor);
PQXX_REGISTER_TEST(test_notice_handler_works_after_moving_connection);
} // namespace
