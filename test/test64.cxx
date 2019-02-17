#include "test_helpers.hxx"

using namespace pqxx;


// Example program for libpqxx.  Test session variables with asyncconnection.
namespace
{
std::string GetDatestyle(connection_base &conn)
{
  return nontransaction(conn, "getdatestyle").get_variable("DATESTYLE");
}


std::string SetDatestyle(connection_base &conn, std::string style)
{
  conn.set_variable("DATESTYLE", style);
  const std::string fullname = GetDatestyle(conn);
  PQXX_CHECK(
	not fullname.empty(),
	"Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void CheckDatestyle(connection_base &conn, std::string expected)
{
  PQXX_CHECK_EQUAL(GetDatestyle(conn), expected, "Got wrong datestyle.");
}


void RedoDatestyle(
	connection_base &conn,
	std::string style,
	std::string expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(conn, style), expected, "Set wrong datestyle.");
}


void ActivationTest(
	connection_base &conn,
	std::string style,
	std::string expected)
{
  RedoDatestyle(conn, style, expected);
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  conn.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>
  CheckDatestyle(conn, expected);
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  conn.activate();
#include <pqxx/internal/ignore-deprecated-post.hxx>
  CheckDatestyle(conn, expected);
}


void test_064()
{
  asyncconnection conn;

  PQXX_CHECK(not GetDatestyle(conn).empty(), "Initial datestyle not set.");

  const std::string ISOname = SetDatestyle(conn, "ISO");
  const std::string SQLname = SetDatestyle(conn, "SQL");

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname, "Same datestyle in SQL and ISO.");

  RedoDatestyle(conn, "SQL", SQLname);

  ActivationTest(conn, "ISO", ISOname);
  ActivationTest(conn, "SQL", SQLname);

  // Prove that setting an unknown variable causes an error, as expected
  quiet_errorhandler d(conn);
  PQXX_CHECK_THROWS(
	conn.set_variable("NONEXISTENT_VARIABLE_I_HOPE", "1"),
	sql_error,
	"Setting unknown variable failed to fail.");
}


PQXX_REGISTER_TEST(test_064);
} // namespace
