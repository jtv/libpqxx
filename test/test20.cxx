#include <iostream>

#include "test_helpers.hxx"

using namespace pqxx;


// Test: nontransaction changes are not rolled back on abort.
namespace
{
const unsigned long BoringYear = 1977;


void test_020()
{
  connection conn;
  nontransaction t1{conn};
  test::create_pqxxevents(t1);

  const std::string Table = "pqxxevents";

  // Verify our start condition before beginning: there must not be a 1977
  // record already.
  result R( t1.exec(("SELECT * FROM " + Table + " "
	               "WHERE year=" + to_string(BoringYear)).c_str()) );
  PQXX_CHECK_EQUAL(
	R.size(),
	0u,
	"Already have a row for " + to_string(BoringYear) + ", cannot test.");

  // (Not needed, but verify that clear() works on empty containers)
  R.clear();
  PQXX_CHECK(R.empty(), "result::clear() is broken.");

  // OK.  Having laid that worry to rest, add a record for 1977.
  t1.exec0(("INSERT INTO " + Table + " VALUES"
           "(" +
	   to_string(BoringYear) + ","
	   "'Yawn'"
	   ")").c_str());

  // Abort T1.  Since T1 is a nontransaction, which provides only the
  // transaction class interface without providing any form of transactional
  // integrity, this is not going to undo our work.
  t1.abort();

  // Verify that our record was added, despite the Abort()
  nontransaction t2{conn, "t2"};
  R = t2.exec(("SELECT * FROM " + Table + " "
	"WHERE year=" + to_string(BoringYear)).c_str());

  PQXX_CHECK_EQUAL(
	R.size(),
	1u,
	"Found wrong number of rows for " + to_string(BoringYear) + ".");

  PQXX_CHECK(R.capacity() >= R.size(), "Result's capacity is too small.");

  R.clear();
  PQXX_CHECK(R.empty(), "result::clear() doesn't work.");

  // Now remove our record again
  t2.exec0(("DELETE FROM " + Table + " "
	   "WHERE year=" + to_string(BoringYear)).c_str());

  t2.commit();

  // And again, verify results
  nontransaction t3{conn, "t3"};

  R = t3.exec(("SELECT * FROM " + Table + " "
	       "WHERE year=" + to_string(BoringYear)).c_str());

  PQXX_CHECK_EQUAL(R.size(), 0u, "Record still found after removal.");
}


PQXX_REGISTER_TEST(test_020);
} // namespace
