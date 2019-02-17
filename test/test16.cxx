#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test robusttransaction.
namespace
{
void test_016()
{
  connection conn;
  robusttransaction<> tx{conn};
  result R{ tx.exec("SELECT * FROM pg_tables") };

  result::const_iterator c;
  for (c = R.begin(); c != R.end(); ++c)
  {
    std::string N;
    c[0].to(N);

    std::cout << '\t' << to_string(c.num()) << '\t' << N << std::endl;
  }

  // See if back() and row comparison work properly
  PQXX_CHECK(R.size() >= 2, "Not enough rows in pg_tables to test, sorry!");

  --c;

  PQXX_CHECK_EQUAL(
	c->size(),
	R.back().size(),
	"Size mismatch between row iterator and back().");

  const std::string nullstr;
  for (pqxx::row::size_type i = 0; i < c->size(); ++i)
    PQXX_CHECK_EQUAL(
	c[i].as(nullstr),
	R.back()[i].as(nullstr),
	"Value mismatch in back().");
    PQXX_CHECK(c == R.back(), "Row equality is broken.");
    PQXX_CHECK(not (c != R.back()), "Row inequality is broken.");

  tx.commit();
}


PQXX_REGISTER_TEST(test_016);
} // namespace
