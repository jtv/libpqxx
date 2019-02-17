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
  nontransaction tx{conn};

  const char databuf[] = "Test\0data";
  const std::string data(databuf, sizeof(databuf));
  PQXX_CHECK(data.size() > strlen(databuf), "Unknown data length problem.");

  const std::string Table = "pqxxbin", Field = "binfield", Stat = "nully";
  tx.exec0("CREATE TEMP TABLE " + Table + " (" + Field + " BYTEA)");

#include <pqxx/internal/ignore-deprecated-pre.hxx>
  conn.prepare(Stat, "INSERT INTO " + Table + " VALUES ($1)");
  tx.prepared(Stat)(binarystring(data)).exec();

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

  // People seem to like the multi-line invocation style, where you get your
  // invocation object first, then add parameters in separate C++ statements.
  // As John Mudd found, that used to break the code.  Let's test it.
  tx.exec0("CREATE TEMP TABLE row (one INTEGER, two VARCHAR)");
  conn.prepare("makerow", "INSERT INTO row VALUES ($1, $2)");

  pqxx::prepare::invocation i{ tx.prepared("makerow") };
  const std::string f = "frobnalicious";
  i(6);
  i(f);
  i.exec();
#include <pqxx/internal/ignore-deprecated-post.hxx>

  const result t{ tx.exec("SELECT * FROM row") };
  PQXX_CHECK_EQUAL(t.size(), 1u, "Wrong result size.");
  PQXX_CHECK_EQUAL(
	t[0][0].as<std::string>(),
	"6",
	"Unexpected result value.");
  PQXX_CHECK_EQUAL(t[0][1].c_str(), f, "Unexpected string result.");
}
} // namespace


PQXX_REGISTER_TEST(test_092);
