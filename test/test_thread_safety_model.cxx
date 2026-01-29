#include "helpers.hxx"

#include <pqxx/util>

namespace
{
void test_thread_safety_model(pqxx::test::context &)
{
  auto const model{pqxx::describe_thread_safety()};

  if (model.safe_libpq and model.safe_kerberos)
    PQXX_CHECK_EQUAL(model.description, "");
  else
    PQXX_CHECK_NOT_EQUAL(model.description, "");
}


PQXX_REGISTER_TEST(test_thread_safety_model);
} // namespace
