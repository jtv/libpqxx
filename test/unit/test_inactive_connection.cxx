#include "../test_helpers.hxx"

namespace
{
void test_inactive_connection()
{
  pqxx::lazyconnection conn;

  PQXX_CHECK_THROWS(
	conn.port(),
	pqxx::broken_connection,
	"No exception calling port() on inactive connection.");

  PQXX_CHECK_THROWS(
	pqxx::work tx{conn},
	pqxx::broken_connection,
	"No exception starting transaction on inactive connection.");

  {
    pqxx::nontransaction tx{conn};
    PQXX_CHECK_THROWS(
	tx.exec("SELECT 0"),
	pqxx::broken_connection,
	"No exception executing query on inactive connection.");
  }

  conn.activate();
  PQXX_CHECK_NOT_EQUAL(conn.port(), nullptr, "No port on active connection.");
}


PQXX_REGISTER_TEST(test_inactive_connection);
}
