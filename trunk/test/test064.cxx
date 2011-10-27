#include <iostream>

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Example program for libpqxx.  Test session variables with asyncconnection.
namespace
{
string GetDatestyle(connection_base &C)
{
  return nontransaction(C, "getdatestyle").get_variable("DATESTYLE");
}


string SetDatestyle(connection_base &C, string style)
{
  C.set_variable("DATESTYLE", style);
  const string fullname = GetDatestyle(C);
  cout << "Set datestyle to " << style << ": " << fullname << endl;
  PQXX_CHECK(
	!fullname.empty(),
	"Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void CheckDatestyle(connection_base &C, string expected)
{
  PQXX_CHECK_EQUAL(GetDatestyle(C), expected, "Got wrong datestyle.");
}


void RedoDatestyle(connection_base &C, string style, string expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(C, style), expected, "Set wrong datestyle.");
}


void ActivationTest(connection_base &C, string style, string expected)
{
  RedoDatestyle(C, style, expected);
  cout << "Deactivating connection..." << endl;
  C.deactivate();
  CheckDatestyle(C, expected);
  cout << "Reactivating connection..." << endl;
  C.activate();
  CheckDatestyle(C, expected);
}


void test_064(transaction_base &)
{
  asyncconnection C;

  PQXX_CHECK(!GetDatestyle(C).empty(), "Initial datestyle not set.");

  const string ISOname = SetDatestyle(C, "ISO");
  const string SQLname = SetDatestyle(C, "SQL");

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname, "Same datestyle in SQL and ISO.");

  RedoDatestyle(C, "SQL", SQLname);

  ActivationTest(C, "ISO", ISOname);
  ActivationTest(C, "SQL", SQLname);

  // Prove that setting an unknown variable causes an error, as expected
  quiet_errorhandler d(C);
  PQXX_CHECK_THROWS(
	C.set_variable("NONEXISTENT_VARIABLE_I_HOPE", "1"),
	sql_error,
	"Setting unknown variable failed to fail.");
}
} // namespace

PQXX_REGISTER_TEST_NODB(test_064)
