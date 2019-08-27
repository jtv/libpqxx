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
  PQXX_CHECK_EQUAL(tx.exec1("SELECT 5")[0].as<int>(), 5, "Weird result!");

  PQXX_CHECK_THROWS(
	pqxx::connection c3{std::move(c2)},
	pqxx::usage_error,
	"Moving a connection with a transaction open should be an error.");
}


PQXX_REGISTER_TEST(test_move_constructor);
}
