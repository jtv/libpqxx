#include <cstring>
#include <iostream>
#include <list>

#include "test_helpers.hxx"


using namespace PGSTD;
using namespace pqxx;

// Test program for libpqxx.  Test binary parameters to prepared statements.

namespace
{
void test_092(transaction_base &T)
{
  const char databuf[] = "Test\0data";
  const string data(databuf, sizeof(databuf));
  PQXX_CHECK(data.size() > strlen(databuf), "Unknown data length problem.");

  const string Table = "pqxxbin", Field = "binfield", Stat = "nully";
  T.exec("CREATE TEMP TABLE " + Table + " (" + Field + " BYTEA)");

  T.conn().prepare(Stat, "INSERT INTO " + Table + " VALUES ($1)");
  T.prepared(Stat)(binarystring(data)).exec();

  const result L( T.exec("SELECT length("+Field+") FROM " + Table) );
  PQXX_CHECK_EQUAL(L[0][0].as<size_t>(),
	data.size(),
	"Length of field in database does not match original length.");

  const result R( T.exec("SELECT " + Field + " FROM " + Table) );

  const binarystring roundtrip(R[0][0]);

  PQXX_CHECK_EQUAL(string(roundtrip.str()), data, "Data came back different.");

  PQXX_CHECK_EQUAL(roundtrip.size(),
	data.size(),
	"Binary string reports wrong size.");

  // People seem to like the multi-line invocation style, where you get your
  // invocation object first, then add parameters in separate C++ statements.
  // As John Mudd found, that used to break the code.  Let's test it.
  T.exec("CREATE TEMP TABLE tuple (one INTEGER, two VARCHAR)");
  T.conn().prepare("maketuple", "INSERT INTO tuple VALUES ($1, $2)");

  pqxx::prepare::invocation i( T.prepared("maketuple") );
  const string f = "frobnalicious";
  i(6);
  i(f);
  i.exec();

  const result t( T.exec("SELECT * FROM tuple") );
  PQXX_CHECK_EQUAL(t.size(), 1u, "Wrong result size.");
  PQXX_CHECK_EQUAL(t[0][0].as<string>(), "6", "Unexpected result value.");
  PQXX_CHECK_EQUAL(t[0][1].c_str(), f, "Unexpected string result.");
}
} // namespace

PQXX_REGISTER_TEST_CT(test_092, lazyconnection, nontransaction)
