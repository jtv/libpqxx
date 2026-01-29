#include <map>

#include <pqxx/transaction>
#include <pqxx/transactor>

#include "helpers.hxx"


// Example program for libpqxx.  Modify the database, retaining transactional
// integrity using the transactor framework.
//
// This assumes the existence of a database table "pqxxevents" containing a
// 2-digit "year" field, which is extended to a 4-digit format by assuming all
// year numbers of 70 or higher are in the 20th century, and all others in the
// 21st, and that no years before 1970 are possible.

namespace
{
void test_007(pqxx::test::context &)
{
  pqxx::connection cx;
  cx.set_client_encoding("SQL_ASCII");

  {
    pqxx::work tx{cx};
    pqxx::test::create_pqxxevents(tx);
    tx.commit();
  }

  // Perform (an instantiation of) the UpdateYears transactor we've defined
  // in the code above.  This is where the work gets done.
  std::map<int, int> conversions;
  pqxx::perform([&conversions, &cx] {
    pqxx::work tx{cx};
    // First select all different years occurring in the table.
    pqxx::result R(tx.exec("SELECT year FROM pqxxevents"));

    // See if we get reasonable type identifier for this column.
    pqxx::oid const rctype{R.column_type(0)};
    PQXX_CHECK_EQUAL(
      R.column_type(pqxx::row::size_type(0)), rctype,
      "Inconsistent result::column_type().");

    std::string const rct{pqxx::to_string(rctype)};
    PQXX_CHECK(rctype > 0, "Got strange type ID for column: " + rct);

    std::string const rcol{R.column_name(0)};
    PQXX_CHECK(not std::empty(rcol));

    pqxx::oid const rcctype{R.column_type(rcol)};
    PQXX_CHECK_EQUAL(rcctype, rctype);

    pqxx::oid const rawrcctype{R.column_type(rcol)};
    PQXX_CHECK_EQUAL(rawrcctype, rctype);

    // Note all different years currently occurring in the table, writing
    // them and their correct mappings to conversions.
    for (auto const &r : R)
    {
      // See if type identifiers are consistent
      pqxx::oid const tctype{r.column_type(0)};

      PQXX_CHECK_EQUAL(tctype, r.column_type(pqxx::row::size_type(0)));

      PQXX_CHECK_EQUAL(tctype, rctype, );

      pqxx::oid const ctctype{r.column_type(rcol)};

      PQXX_CHECK_EQUAL(ctctype, rctype);

      pqxx::oid const rawctctype{r.column_type(rcol)};

      PQXX_CHECK_EQUAL(rawctctype, rctype);

      pqxx::oid const fctype{r[0].type()};
      PQXX_CHECK_EQUAL(fctype, rctype);
    }

    // For each occurring year, write converted date back to whereever it may
    // occur in the table.  Since we're in a transaction, any changes made by
    // others at the same time will not affect us.
    for (auto const &c : conversions)
    {
      auto const query{
        "UPDATE pqxxevents "
        "SET year=" +
        pqxx::to_string(c.second) +
        " "
        "WHERE year=" +
        pqxx::to_string(c.first)};
      R = tx.exec(query).no_rows();
    }
  });
}


PQXX_REGISTER_TEST(test_007);
} // namespace
