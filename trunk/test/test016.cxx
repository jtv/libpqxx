#include <iostream>

#include <pqxx/connection>
#include <pqxx/robusttransaction>
#include <pqxx/result>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{

void test_016(connection_base &, transaction_base &T)
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
  if (R.size() < 2)
    throw runtime_error("Not enough results in pg_tables to test, sorry!");
  --c;
  if (c->size() != R.back().size())
    throw logic_error("Size mismatch between tuple iterator and back()");
  const string nullstr;
  for (result::tuple::size_type i = 0; i < c->size(); ++i)
    if (c[i].as(nullstr) != R.back()[i].as(nullstr))
	throw logic_error("Value mismatch in back()");
  if (c != R.back())
    throw logic_error("Something wrong with tuple inequality");
  if (!(c == R.back()))
    throw logic_error("Something wrong with tuple equality");

  // "Commit" the non-transaction.  This doesn't really do anything since
  // NonTransaction doesn't start a backend transaction.
  T.commit();
}
} // namespace

PQXX_REGISTER_TEST_T(test_016, robusttransaction<>)
