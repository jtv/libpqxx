#include <cmath>
#include <iostream>
#include <stdexcept>

#include <pqxx/connection>
#include <pqxx/result>
#include <pqxx/transaction>

using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test fieldstream.
//
// Usage: test074 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a backend 
// running on host foo.bar.net, logging in as user smith.
int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    work W(C, "test74");
    result R = W.exec("SELECT * FROM pg_tables");
    const string sval = R.at(0).at(1).c_str();
    string sval2;
    fieldstream fs1(R[0][1]);
    fs1 >> sval2;
    if (sval2 != sval)
      throw logic_error("Got '" + sval + "' from field, "
	  "but '" + sval2 + "' from fieldstream");

    R = W.exec("SELECT count(*) FROM pg_tables");
    int ival;
    fieldstream fs2(R.at(0).at(0));
    fs2 >> ival;
    if (ival != R[0][0].as<int>())
      throw logic_error("Got int " + ToString(ival) + " from fieldstream, "
	  "but " + ToString(R[0][0].as<int>()) + " from field");

    double dval;
    (fieldstream(R.at(0).at(0))) >> dval;
    if (fabs(dval - R[0][0].as<double>()) > 0.1)
      throw logic_error("Got double " + ToString(dval) + " from fieldstream, "
	  "but " + ToString(R[0][0].as<double>()) + " from field");

    const float roughpi = 3.1415926435;
    R = W.exec("SELECT " + ToString(roughpi));
    float pival;
    (fieldstream(R.at(0).at(0))) >> pival;
    if (fabs(pival - roughpi) > 0.001)
      throw logic_error("Pi approximation came back as " + ToString(roughpi));
  }
  catch (const sql_error &e)
  {
    cerr << "Database error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 2;
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

