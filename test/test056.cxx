#include <iostream>

#include <pqxx/pqxx>

// Don't try this at home: use some libpqxx-internal information about libpq
#include <pqxx/config-internal-libpq.h>

using namespace PGSTD;
using namespace pqxx;


// Simple test program for libpqxx.  Issue invalid query and handle error.
//
// Usage: test056
int main()
{
  try
  {
    connection C;
    work T(C, "test56");

    try
    {
      disable_noticer d(C);

      // This should fail:
      T.exec("DELIBERATELY INVALID TEST QUERY...", "invalid_query");

      throw logic_error("Invalid query did not throw exception!");
    }
#if defined(PQXX_HAVE_PQRESULTERRORFIELD)
    catch (const syntax_error &e)
#else
    catch (const sql_error &e)
#endif
    {
      cout << "(Expected) Query failed: " << e.query() << endl
	   << "(Expected) Error was: " << e.what() << endl;
    }
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


