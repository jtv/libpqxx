#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
void test_016(transaction_base &T)
{
  result R( T.exec("SELECT * FROM pg_tables") );

  result::const_iterator c;
  for (c = R.begin(); c != R.end(); ++c)
  {
    string N;
    c[0].to(N);

    cout << '\t' << to_string(c.num()) << '\t' << N << endl;
  }

  // See if back() and tuple comparison work properly
  PQXX_CHECK(R.size() >= 2, "Not enough rows in pg_tables to test, sorry!");

  --c;

  PQXX_CHECK_EQUAL(
	c->size(),
	R.back().size(),
	"Size mismatch between tuple iterator and back().");

  const string nullstr;
  for (tuple::size_type i = 0; i < c->size(); ++i)
    PQXX_CHECK_EQUAL(
	c[i].as(nullstr),
	R.back()[i].as(nullstr),
	"Value mismatch in back().");
    PQXX_CHECK(c == R.back(), "Tuple equality is broken.");
    PQXX_CHECK(!(c != R.back()), "Tuple inequality is broken.");

  // "Commit" the non-transaction.  This doesn't really do anything since
  // NonTransaction doesn't start a backend transaction.
  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_016, robusttransaction<>)
