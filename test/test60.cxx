#include <iostream>

#include <pqxx/nontransaction>
#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace pqxx;


// Example program for libpqxx.  Test session variable functionality.
namespace
{
std::string GetDatestyle(connection &cx)
{
  return cx.get_var("DATESTYLE");
}


std::string SetDatestyle(connection &cx, std::string style)
{
  cx.set_session_var("DATESTYLE", style);
  std::string const fullname{GetDatestyle(cx)};
  PQXX_CHECK(
    not std::empty(fullname),
    "Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void CheckDatestyle(connection &cx, std::string expected)
{
  PQXX_CHECK_EQUAL(GetDatestyle(cx), expected, "Got wrong datestyle.");
}


void RedoDatestyle(
  connection &cx, std::string const &style, std::string const &expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(cx, style), expected, "Set wrong datestyle.");
}


void ActivationTest(
  connection &cx, std::string const &style, std::string const &expected)
{
  RedoDatestyle(cx, style, expected);
  CheckDatestyle(cx, expected);
}


void test_060()
{
  connection cx;

  PQXX_CHECK(not std::empty(GetDatestyle(cx)), "Initial datestyle not set.");

  std::string const ISOname{SetDatestyle(cx, "ISO")};
  std::string const SQLname{SetDatestyle(cx, "SQL")};

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname, "Same datestyle in SQL and ISO.");

  RedoDatestyle(cx, "SQL", SQLname);

  ActivationTest(cx, "ISO", ISOname);
  ActivationTest(cx, "SQL", SQLname);

  PQXX_CHECK_THROWS(
    cx.set_session_var("bonjour_name", std::optional<std::string>{}),
    pqxx::variable_set_to_null,
    "Setting a variable to null did not report the error correctly.");

  // Prove that setting an unknown variable causes an error, as expected
  quiet_errorhandler d{cx};
  PQXX_CHECK_THROWS(
    cx.set_session_var("NONEXISTENT_VARIABLE_I_HOPE", 1), sql_error,
    "Setting unknown variable failed to fail.");
}


PQXX_REGISTER_TEST(test_060);
} // namespace
