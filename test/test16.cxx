#include <pqxx/connection>
#include <pqxx/robusttransaction>

#include "helpers.hxx"


// Test robusttransaction.
namespace
{
void test_016()
{
  pqxx::connection cx;
  pqxx::robusttransaction<> tx{cx};
  pqxx::result r{tx.exec("SELECT * FROM pg_tables")};

  pqxx::result::const_iterator c;
  for (c = std::begin(r); c != std::end(r); ++c);

  // See if back() and row comparison work properly
  PQXX_CHECK(
    std::size(r) >= 2, "Not enough rows in pg_tables to test, sorry!");

  --c;

  PQXX_CHECK_EQUAL(c->size(), std::size(r.back()));

  std::string const nullstr;
  for (pqxx::row::size_type i{0}; i < c->size(); ++i)
    PQXX_CHECK_EQUAL((*c)[i].as(nullstr), r.back()[i].as(nullstr));
}


PQXX_REGISTER_TEST(test_016);
} // namespace
