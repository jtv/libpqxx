#include <pqxx/nontransaction>

#include "helpers.hxx"


// Test: nontransaction changes are committed immediately.
namespace
{
constexpr int boring_year_39{1977};


void test_039(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::nontransaction tx1{cx};
  pqxx::test::create_pqxxevents(tx1);
  std::string const table{"pqxxevents"};

  // Verify our start condition before beginning: there must not be a 1977
  // record already.
  pqxx::result r(tx1.exec(
    "SELECT * FROM " + table +
    " "
    "WHERE year=" +
    pqxx::to_string(boring_year_39)));

  PQXX_CHECK_EQUAL(
    std::size(r), 0,
    "Already have a row for " + pqxx::to_string(boring_year_39) +
      ", cannot test.");

  // (Not needed, but verify that clear() works on empty containers)
  r.clear();
  PQXX_CHECK(std::empty(r));

  // OK.  Having laid that worry to rest, add a record for 1977.
  tx1
    .exec(
      "INSERT INTO " + table +
      " VALUES"
      "(" +
      pqxx::to_string(boring_year_39) +
      ","
      "'Yawn'"
      ")")
    .no_rows();

  // Abort tx1.  Since tx1 is a nontransaction, which provides only the
  // transaction class interface without providing any form of transactional
  // integrity, this is not going to undo our work.
  tx1.abort();

  // Verify that our record was added, despite the Abort()
  pqxx::nontransaction tx2(cx, "tx2");
  r = tx2.exec(
    "SELECT * FROM " + table +
    " "
    "WHERE year=" +
    pqxx::to_string(boring_year_39));
  PQXX_CHECK_EQUAL(std::size(r), 1);

  PQXX_CHECK_GREATER_EQUAL(r.capacity(), std::size(r));

  r.clear();
  PQXX_CHECK(std::empty(r));

  // Now remove our record again
  tx2
    .exec(
      "DELETE FROM " + table +
      " "
      "WHERE year=" +
      pqxx::to_string(boring_year_39))
    .no_rows();

  tx2.commit();

  // And again, verify results
  pqxx::nontransaction tx3(cx, "tx3");

  r = tx3.exec(
    "SELECT * FROM " + table +
    " "
    "WHERE year=" +
    pqxx::to_string(boring_year_39));

  PQXX_CHECK_EQUAL(std::size(r), 0);
}


PQXX_REGISTER_TEST(test_039);
} // namespace
