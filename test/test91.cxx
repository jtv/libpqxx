#include <pqxx/transaction>
#include <pqxx/row>
#include <string>

#include "test_helpers.hxx"

using namespace pqxx;

namespace{
void test_091(){
  //create a connection to the database
  std::string connString;
  pqxx::connection cx {connString};
  //create a database transaction
  pqxx::work tx{cx};

  tx.exec("DROP TABLE IF EXISTS test_tuple");
  tx.exec("CREATE TABLE test_tuple (id INTEGER, name TEXT)");
  tx.exec("INSERT INTO test_tuple VALUES (1, 'Alice')");
  pqxx::result R(tx.exec("SELECT id, name FROM test_tuple"));
  tx.exec("DROP TABLE IF EXISTS test_tuple");

  //commit the transaction
  tx.commit();

  //correct_tuple_t matches the row size and signature
  using correct_tuple_t = std::tuple<int, std::string>;
  //incorrect_tuple_t does not matches the row size and signature
  using incorrect_tuple_t = std::tuple<int>;

  //check that the size of the returned result is as expected
  //since we only inserted one row in the 'test_tuple' table.
  PQXX_CHECK_EQUAL(R.size(), 1, "Unexpected size from pqxx::result");
  correct_tuple_t t = R[0].as_tuple<correct_tuple_t>();

  //check that tuple elements contain the expected values
  PQXX_CHECK_EQUAL(std::get<0>(t), 1, "Incorrect type for tuple value 0");
  PQXX_CHECK_EQUAL(std::get<1>(t), "Alice", "Incorrect type for tuple value 1");

  //check that when incorrect_tuple_t is used, the expected exception is thrown
  PQXX_CHECK_THROWS(R[0].as_tuple<incorrect_tuple_t>(), pqxx::usage_error,
                    "pqxx::row::as_tuple does not throw expected exception for incorrect tuple type");
}

PQXX_REGISTER_TEST(test_091);
}//namespace
