#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.
namespace
{
class ReadTables : public transactor<nontransaction>
{
  result m_result;
public:
  ReadTables() : transactor<nontransaction>("ReadTables") {}

  void operator()(argument_type &T)
  {
    m_result = T.exec("SELECT * FROM pg_tables");
  }

  void on_commit()
  {
    for (const auto &c: m_result)
    {
      string N;
      c[0].to(N);

      cout << '\t' << to_string(c.num()) << '\t' << N << endl;
    }
  }
};


void test_036(transaction_base &)
{
  lazyconnection C;
  C.perform(ReadTables());
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_036)
