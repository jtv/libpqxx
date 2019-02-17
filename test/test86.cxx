#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test inhibition of connection reactivation
namespace
{
void test_086()
{
  connection conn;
  nontransaction tx1{conn};

  const std::string Query = "SELECT * from pg_tables";

  std::cout << "Some datum: " << tx1.exec(Query)[0][0] << std::endl;
  tx1.commit();

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  conn.inhibit_reactivation(true);
  conn.deactivate();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  quiet_errorhandler d{conn};
  {
    nontransaction tx2{conn, "tx2"};
    PQXX_CHECK_THROWS(
	tx2.exec(Query),
	broken_connection,
	"Deactivated connection did not throw broken_connection on exec().");
  }

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  conn.inhibit_reactivation(false);
#include <pqxx/internal/ignore-deprecated-post.hxx>
  
  work tx3{conn, "tx3"};
  tx3.exec(Query);
  tx3.commit();
}
} // namespace


PQXX_REGISTER_TEST(test_086);
