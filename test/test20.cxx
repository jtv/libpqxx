#include <pqxx/connection>
#include <pqxx/nontransaction>

#include "helpers.hxx"


// Test: nontransaction changes are not rolled back on abort.
namespace
{
constexpr int boring_year_20{1977};


void test_020(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::nontransaction t1{cx};
  pqxx::test::create_pqxxevents(t1);

  std::string const table{"pqxxevents"};

  // Verify our start condition before beginning: there must not be a 1977
  // record already.
  pqxx::result r{t1.exec(
    "SELECT * FROM " + table +
    " WHERE year=" + pqxx::to_string(boring_year_20))};
  PQXX_CHECK_EQUAL(
    std::size(r), 0,
    "Already have a row for " + pqxx::to_string(boring_year_20) +
      ", cannot test.");

  // (Not needed, but verify that clear() works on empty containers)
  r.clear();
  PQXX_CHECK(std::empty(r));

  // OK.  Having laid that worry to rest, add a record for 1977.
  t1.exec(
      "INSERT INTO " + table +
      " VALUES"
      "(" +
      pqxx::to_string(boring_year_20) +
      ","
      "'Yawn'"
      ")")
    .no_rows();

  // Abort T1.  Since T1 is a nontransaction, which provides only the
  // transaction class interface without providing any form of transactional
  // integrity, this is not going to undo our work.
  t1.abort();

  // Verify that our record was added, despite the Abort()
  pqxx::nontransaction t2{cx, "t2"};
  r = t2.exec(
    "SELECT * FROM " + table +
    " WHERE year=" + pqxx::to_string(boring_year_20));

  PQXX_CHECK_EQUAL(std::size(r), 1);

  PQXX_CHECK_GREATER_EQUAL(r.capacity(), std::size(r));

  r.clear();
  PQXX_CHECK(std::empty(r));

  // Now remove our record again
  t2.exec(
      "DELETE FROM " + table +
      " "
      "WHERE year=" +
      pqxx::to_string(boring_year_20))
    .no_rows();

  t2.commit();

  // And again, verify results
  pqxx::nontransaction t3{cx, "t3"};

  r = t3.exec(
    "SELECT * FROM " + table +
    " WHERE year=" + pqxx::to_string(boring_year_20));

  PQXX_CHECK_EQUAL(std::size(r), 0);
}


PQXX_REGISTER_TEST(test_020);
} // namespace
