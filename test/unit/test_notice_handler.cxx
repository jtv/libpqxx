#include <pqxx/transaction>

#include "../test_helpers.hxx"
#include<iostream>// XXX: DEBUG

namespace
{
void test_notice_handler_receives_notice()
{
  pqxx::connection cx;
  int notices{0};

  cx.set_notice_handler(
    [&notices](pqxx::zview) noexcept
    {
      ++notices;
    }
  );

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
  cx.set_notice_handler([&calls, &received](auto x){ ++calls; received = x; });
  cx.process_notice(msg);

  PQXX_CHECK_EQUAL(calls, 1, "Expected exactly 1 call to notice handler.");
  PQXX_CHECK_EQUAL(received, msg, "Got wrong message.");
}


// XXX: Test stateless lambda.
// XXX: Test function.
// XXX: Test functor by value.
// XXX: Test functor by reference.
// XXX: Test functor by rvalue reference.
// XXX: Test processing after copy/move construction/assignment.


PQXX_REGISTER_TEST(test_notice_handler_receives_notice);
PQXX_REGISTER_TEST(test_notice_handler_works_after_connection_closes);
PQXX_REGISTER_TEST(test_process_notice_calls_notice_handler);
} // namespace
