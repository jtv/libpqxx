#include <pqxx/transaction>

#include "helpers.hxx"


// Example program for libpqxx.  Test local variable functionality.
namespace
{
std::string GetDatestyle(pqxx::transaction_base &T)
{
  return T.conn().get_var("DATESTYLE");
}


std::string SetDatestyle(pqxx::transaction_base &T, std::string const &style)
{
  T.conn().set_session_var("DATESTYLE", style);
  std::string const fullname{GetDatestyle(T)};
  PQXX_CHECK(
    not std::empty(fullname),
    "Setting datestyle to " + style + " makes it an empty string.");

  return fullname;
}


void RedoDatestyle(
  pqxx::transaction_base &T, std::string const &style,
  std::string const &expected)
{
  PQXX_CHECK_EQUAL(SetDatestyle(T, style), expected);
}


void test_061(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  PQXX_CHECK(not std::empty(GetDatestyle(tx)));

  std::string const ISOname{SetDatestyle(tx, "ISO")};
  std::string const SQLname{SetDatestyle(tx, "SQL")};

  PQXX_CHECK_NOT_EQUAL(ISOname, SQLname);

  RedoDatestyle(tx, "SQL", SQLname);

  // Prove that setting an unknown variable causes an error, as expected
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::quiet_errorhandler const d(tx.conn());
#include "pqxx/internal/ignore-deprecated-post.hxx"
  PQXX_CHECK_THROWS(
    cx.set_session_var("NONEXISTENT_VARIABLE_I_HOPE", 1), pqxx::sql_error);
}


PQXX_REGISTER_TEST(test_061);
} // namespace
