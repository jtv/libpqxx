#include <pqxx/nontransaction>
#include <pqxx/pipeline>
#include <pqxx/transaction>

#include "helpers.hxx"


// Example program for libpqxx.  Test session variable functionality.
namespace
{
std::string get_datestyle(pqxx::connection &cx)
{
  return cx.get_var("DATESTYLE");
}


std::string set_datestyle(pqxx::connection &cx, std::string const &style)
{
  cx.set_session_var("DATESTYLE", style);
  std::string const fullname{get_datestyle(cx)};
  PQXX_CHECK(
    not std::empty(fullname),
    "Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void check_datestyle(pqxx::connection &cx, std::string const &expected)
{
  PQXX_CHECK_EQUAL(get_datestyle(cx), expected);
}


void redo_datestyle(
  pqxx::connection &cx, std::string const &style, std::string const &expected)
{
  PQXX_CHECK_EQUAL(set_datestyle(cx, style), expected);
}


void check_setting_datestyle(
  pqxx::connection &cx, std::string const &style, std::string const &expected)
{
  redo_datestyle(cx, style, expected);
  check_datestyle(cx, expected);
}


void test_060(pqxx::test::context &)
{
  pqxx::connection cx;

  PQXX_CHECK(not std::empty(get_datestyle(cx)));

  std::string const ISOname{set_datestyle(cx, "ISO")};
  std::string const SQLname{set_datestyle(cx, "SQL")};

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname);

  redo_datestyle(cx, "SQL", SQLname);

  check_setting_datestyle(cx, "ISO", ISOname);
  check_setting_datestyle(cx, "SQL", SQLname);

  PQXX_CHECK_THROWS(
    cx.set_session_var("bonjour_name", std::optional<std::string>{}),
    pqxx::variable_set_to_null);

  // Prove that setting an unknown variable causes an error, as expected
  PQXX_CHECK_THROWS(
    cx.set_session_var("NONEXISTENT_VARIABLE_I_HOPE", 1), pqxx::sql_error);
}


PQXX_REGISTER_TEST(test_060);
} // namespace
