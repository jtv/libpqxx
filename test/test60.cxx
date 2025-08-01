#include <pqxx/nontransaction>
#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"


// Example program for libpqxx.  Test session variable functionality.
namespace
{
std::string GetDatestyle(pqxx::connection &cx)
{
  return cx.get_var("DATESTYLE");
}


std::string SetDatestyle(pqxx::connection &cx, std::string const &style)
{
  cx.set_session_var("DATESTYLE", style);
  std::string const fullname{GetDatestyle(cx)};
  PQXX_CHECK(
    not std::empty(fullname),
    "Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void CheckDatestyle(pqxx::connection &cx, std::string const &expected)
{
  PQXX_CHECK_EQUAL(GetDatestyle(cx), expected);
}


void RedoDatestyle(
  pqxx::connection &cx, std::string const &style, std::string const &expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(cx, style), expected);
}


void ActivationTest(
  pqxx::connection &cx, std::string const &style, std::string const &expected)
{
  RedoDatestyle(cx, style, expected);
  CheckDatestyle(cx, expected);
}


void test_060()
{
  pqxx::connection cx;

  PQXX_CHECK(not std::empty(GetDatestyle(cx)));

  std::string const ISOname{SetDatestyle(cx, "ISO")};
  std::string const SQLname{SetDatestyle(cx, "SQL")};

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname);

  RedoDatestyle(cx, "SQL", SQLname);

  ActivationTest(cx, "ISO", ISOname);
  ActivationTest(cx, "SQL", SQLname);

  PQXX_CHECK_THROWS(
    cx.set_session_var("bonjour_name", std::optional<std::string>{}),
    pqxx::variable_set_to_null);

  // Prove that setting an unknown variable causes an error, as expected
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::quiet_errorhandler const d{cx};
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_THROWS(
    cx.set_session_var("NONEXISTENT_VARIABLE_I_HOPE", 1), pqxx::sql_error);
}


PQXX_REGISTER_TEST(test_060);
} // namespace
