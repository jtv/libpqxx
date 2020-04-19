#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_move_constructor()
{
  pqxx::connection c1;
  PQXX_CHECK(c1.is_open(), "New connection is not open.");

  pqxx::connection c2{std::move(c1)};

  PQXX_CHECK(not c1.is_open(), "Moving did not close original connection.");
  PQXX_CHECK(c2.is_open(), "Moved constructor is not open.");

  pqxx::work tx{c2};
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 5"), 5, "Weird result!");

  PQXX_CHECK_THROWS(
    pqxx::connection c3{std::move(c2)}, pqxx::usage_error,
    "Moving a connection with a transaction open should be an error.");
}


void test_move_assign()
{
  pqxx::connection c1;
  pqxx::connection c2;

  c2.close();

  c2 = std::move(c1);

  PQXX_CHECK(not c1.is_open(), "Connection still open after being moved out.");
  PQXX_CHECK(c2.is_open(), "Moved constructor is not open.");

  {
    pqxx::work tx1{c2};
    PQXX_CHECK_EQUAL(tx1.query_value<int>("SELECT 8"), 8, "What!?");

    pqxx::connection c3;
    PQXX_CHECK_THROWS(
      c3 = std::move(c2), pqxx::usage_error,
      "Moving a connection with a transaction open should be an error.");

    PQXX_CHECK_THROWS(
      c2 = std::move(c3), pqxx::usage_error,
      "Moving a connection onto one with a transaction open should be "
      "an error.");
  }

  // After failed move attempts, the connection is still usable.
  pqxx::work tx2{c2};
  PQXX_CHECK_EQUAL(tx2.query_value<int>("SELECT 6"), 6, "Huh!?");
}


void test_encrypt_password()
{
  pqxx::connection c;
  auto pw{c.encrypt_password("user", "password")};
  PQXX_CHECK(not pw.empty(), "Encrypted password was empty.");
  PQXX_CHECK_EQUAL(
    std::strlen(pw.c_str()), pw.size(),
    "Encrypted password contains a null byte.");
}


void test_connection_string()
{
  pqxx::connection c;
  std::string const connstr{c.connection_string()};

  if (std::getenv("PGUSER") == nullptr)
  {
    PQXX_CHECK(
      connstr.find("user=" + std::string{c.username()}) != std::string::npos,
      "Connection string did not specify user name: " + connstr);
  }
  else
  {
    PQXX_CHECK(
      connstr.find("user=" + std::string{c.username()}) == std::string::npos,
      "Connection string specified user name, even when using default: " +
        connstr);
  }
}


PQXX_REGISTER_TEST(test_move_constructor);
PQXX_REGISTER_TEST(test_move_assign);
PQXX_REGISTER_TEST(test_encrypt_password);
PQXX_REGISTER_TEST(test_connection_string);
} // namespace
