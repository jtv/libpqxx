#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/transaction>


using namespace PGSTD;
using namespace pqxx;

// Test inhibition of connection reactivation
//
// Usage: test086
int main()
{
  try
  {
    const string Query = "SELECT * from pg_tables";

    connection C;

    nontransaction N1(C, "test86N1");
    cout << "Some datum: " << N1.exec(Query)[0][0] << endl;
    N1.commit();

    C.inhibit_reactivation(true);
    C.deactivate();

    try
    {
      nontransaction N2(C, "test86N2");
      N2.exec(Query);
    }
    catch (const broken_connection &e)
    {
      cout << "(Expected) " << e.what() << endl;
    }
    catch (const exception &)
    {
      cerr << "Expected broken_connection, got different exception!" << endl;
      throw;
    }

    C.inhibit_reactivation(false);
    work W(C, "test86W");
    W.exec(Query);
    W.commit();
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
	 << "Query was: " << e.query() << endl;
    return 1;
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 1;
  }

  return 0;
}

