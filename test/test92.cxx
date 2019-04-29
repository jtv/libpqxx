#include <cstring>
#include <iostream>
#include <list>

#include "test_helpers.hxx"


using namespace pqxx;

// Test program for libpqxx.  Test binary parameters to prepared statements.

namespace
{
void test_092()
{
  lazyconnection conn;
  conn.activate();
  nontransaction tx{conn};

  const char databuf[] = "Test\0data";
  const std::string data(databuf, sizeof(databuf));
  PQXX_CHECK(data.size() > strlen(databuf), "Unknown data length problem.");

  const std::string Table = "pqxxbin", Field = "binfield", Stat = "nully";
  tx.exec0("CREATE TEMP TABLE " + Table + " (" + Field + " BYTEA)");

  conn.prepare(Stat, "INSERT INTO " + Table + " VALUES ($1)");
  tx.exec_prepared(Stat, binarystring{data});

  const result L{ tx.exec("SELECT length("+Field+") FROM " + Table) };
  PQXX_CHECK_EQUAL(L[0][0].as<size_t>(),
	data.size(),
	"Length of field in database does not match original length.");

  const result R{ tx.exec("SELECT " + Field + " FROM " + Table) };

  const binarystring roundtrip(R[0][0]);

  PQXX_CHECK_EQUAL(
	std::string{roundtrip.str()},
	data,
	"Data came back different.");

  PQXX_CHECK_EQUAL(roundtrip.size(),
	data.size(),
	"Binary string reports wrong size.");
}
} // namespace


PQXX_REGISTER_TEST(test_092);
