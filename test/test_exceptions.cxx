#include <pqxx/except>
#include <pqxx/transaction>

#include "helpers.hxx"


namespace
{
void test_exceptions(pqxx::test::context &)
{
  std::string const broken_query{"SELECT HORRIBLE ERROR"},
    err{"Error message"};

  try
  {
    throw pqxx::sql_error{err, broken_query};
  }
  catch (std::exception const &e)
  {
    PQXX_CHECK_EQUAL(e.what(), err);
    auto downcast{dynamic_cast<pqxx::sql_error const *>(&e)};
    PQXX_CHECK(downcast != nullptr);
    PQXX_CHECK_EQUAL(downcast->query(), broken_query);
  }

  pqxx::connection cx;
  pqxx::work tx{cx};
  try
  {
    tx.exec("INVALID QUERY HERE");
  }
  catch (pqxx::syntax_error const &e)
  {
    // SQL syntax error has sqlstate error 42601.
    PQXX_CHECK_EQUAL(e.sqlstate(), "42601");
  }
}


PQXX_REGISTER_TEST(test_exceptions);
} // namespace
