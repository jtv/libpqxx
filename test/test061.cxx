#include <iostream>

#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Example program for libpqxx.  Test local variable functionality.
namespace
{
string GetDatestyle(transaction_base &T)
{
  return T.get_variable("DATESTYLE");
}


string SetDatestyle(transaction_base &T, string style)
{
  T.set_variable("DATESTYLE", style);
  const string fullname = GetDatestyle(T);
  cout << "Set datestyle to " << style << ": " << fullname << endl;
  PQXX_CHECK(
	!fullname.empty(),
	"Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void RedoDatestyle(transaction_base &T, string style, string expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(T, style), expected, "Set wrong datestyle.");
}


void test_061(transaction_base &T)
{
  PQXX_CHECK(!GetDatestyle(T).empty(), "Initial datestyle not set.");

  const string ISOname = SetDatestyle(T, "ISO");
  const string SQLname = SetDatestyle(T, "SQL");

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname, "Same datestyle in SQL and ISO.");

  RedoDatestyle(T, "SQL", SQLname);

   // Prove that setting an unknown variable causes an error, as expected
  quiet_errorhandler d(T.conn());
  PQXX_CHECK_THROWS(
	T.set_variable("NONEXISTENT_VARIABLE_I_HOPE", "1"),
	sql_error,
	"Setting unknown variable failed to fail.");
}
} // namespace

PQXX_REGISTER_TEST(test_061)
