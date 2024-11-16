#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_notice_handler_receives_notice()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  int notices{0};

  cx.set_notice_handler([&notices](pqxx::zview){ ++notices; });

  // Trigger a notice.
  PQXX_CHECK_THROWS(
    tx.exec("SELECT foo bar splat").no_rows(),
    pqxx::sql_error,
    "Did not trigger expected exception.");

  PQXX_CHECK_EQUAL(notices, 1, "Did not get expected single notice.");
}


void test_notice_handler_works_after_connection_closes()
{
  pqxx::result r;
  int notices{0};

  {
    pqxx::connection cx;
    cx.set_notice_handler([&notices](pqxx::zview){ ++notices; });
    pqxx::work tx{cx};
    r = tx.exec("SELECT 1");
  }

  PQXX_CHECK_EQUAL(notices, 0, "Got premature notice.");

  // Trigger a notice.
  PQXX_CHECK_THROWS(
    pqxx::ignore_unused(r.at(99).at(00)),
    pqxx::range_error,
    "Did not trigger expected exception.");

  PQXX_CHECK_EQUAL(
    notices, 1, "Did not get expected single post-connection notice.");
}


PQXX_REGISTER_TEST(test_notice_handler_receives_notice);
PQXX_REGISTER_TEST(test_notice_handler_works_after_connection_closes);
} // namespace
