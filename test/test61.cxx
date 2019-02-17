#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Example program for libpqxx.  Test local variable functionality.
namespace
{
std::string GetDatestyle(transaction_base &T)
{
  return T.get_variable("DATESTYLE");
}


std::string SetDatestyle(transaction_base &T, std::string style)
{
  T.set_variable("DATESTYLE", style);
  const std::string fullname = GetDatestyle(T);
  std::cout << "Set datestyle to " << style << ": " << fullname << std::endl;
  PQXX_CHECK(
	not fullname.empty(),
	"Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void RedoDatestyle(transaction_base &T, std::string style, std::string expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(T, style), expected, "Set wrong datestyle.");
}


void test_061()
{
  connection conn;
  work tx{conn};

  PQXX_CHECK(not GetDatestyle(tx).empty(), "Initial datestyle not set.");

  const std::string ISOname = SetDatestyle(tx, "ISO");
  const std::string SQLname = SetDatestyle(tx, "SQL");

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname, "Same datestyle in SQL and ISO.");

  RedoDatestyle(tx, "SQL", SQLname);

   // Prove that setting an unknown variable causes an error, as expected
  quiet_errorhandler d(tx.conn());
  PQXX_CHECK_THROWS(
	tx.set_variable("NONEXISTENT_VARIABLE_I_HOPE", "1"),
	sql_error,
	"Setting unknown variable failed to fail.");
}


PQXX_REGISTER_TEST(test_061);
} // namespace
