#include <pqxx/transaction>

#include "helpers.hxx"

#include <pqxx/internal/header-pre.hxx>
extern "C"
{
#include <libpq-fe.h>
}
#include <pqxx/internal/header-post.hxx>

namespace
{
void test_error_verbosity(pqxx::test::context &)
{
  // The error_verbosity enum matches the PGVerbosity one.  We just don't
  // want to import the latter into our users' namespace.
  PQXX_CHECK_EQUAL(
    static_cast<int>(pqxx::error_verbosity::terse),
    static_cast<int>(PQERRORS_TERSE));
  PQXX_CHECK_EQUAL(
    static_cast<int>(pqxx::error_verbosity::normal),
    static_cast<int>(PQERRORS_DEFAULT));
  PQXX_CHECK_EQUAL(
    static_cast<int>(pqxx::error_verbosity::verbose),
    static_cast<int>(PQERRORS_VERBOSE));

  pqxx::connection cx;
  pqxx::work tx{cx};
  cx.set_verbosity(pqxx::error_verbosity::terse);
  tx.exec("SELECT 1").one_row();
  cx.set_verbosity(pqxx::error_verbosity::verbose);
  tx.exec("SELECT 2").one_row();
}


PQXX_REGISTER_TEST(test_error_verbosity);
} // namespace
