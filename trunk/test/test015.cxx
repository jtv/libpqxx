#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/transactor>
#include <pqxx/result>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Open connection to database, start
// a dummy transaction to gain nontransactional access, and perform a query.

namespace
{
class ReadTables : public transactor<nontransaction>
{
  result m_Result;
public:
  ReadTables() : transactor<nontransaction>("ReadTables") {}

  void operator()(argument_type &T)
  {
    m_Result = T.exec("SELECT * FROM pg_tables");
  }

  void on_commit()
  {
    for (result::const_iterator c = m_Result.begin(); c != m_Result.end(); ++c)
    {
      string N;
      c[0].to(N);

      cout << '\t' << to_string(c.num()) << '\t' << N << endl;
    }
  }
};


void test_015(connection_base &C, transaction_base &ortT)
{
  ortT.abort();

  // See if Deactivate() behaves...
  C.deactivate();

  C.perform(ReadTables());
}

} // namespace

PQXX_REGISTER_TEST_T(test_015, nontransaction)
