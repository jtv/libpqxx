#include <map>

#include <pqxx/nontransaction>
#include <pqxx/transaction>
#include <pqxx/transactor>

#include "helpers.hxx"


// Example program for libpqxx.  Modify the database, retaining transactional
// integrity using the transactor framework.
namespace
{
// Convert year to 4-digit format.
int to_4_digits(int y)
{
  int Result{y};

  PQXX_CHECK(y >= 0, "Negative year: " + pqxx::to_string(y));

  if (y < 70)
    Result += 2000;
  else if (y < 100)
    Result += 1900;
  else
    PQXX_CHECK_GREATER_EQUAL(y, 1970);
  return Result;
}


// Transaction definition for year-field update.  Returns conversions done.
std::map<int, int> update_years(pqxx::connection &cx)
{
  std::map<int, int> conversions;
  pqxx::work tx{cx};

  // Note all different years currently occurring in the table, writing them
  // and their correct mappings to m_conversions
  for (auto const &[y] :
       tx.stream<std::optional<int>>("SELECT year FROM pqxxevents"))
  {
    // Read year, and if it is non-null, note its converted value
    if (bool(y))
      conversions[y.value_or(1)] = to_4_digits(y.value_or(2));
  }

  // For each occurring year, write converted date back to whereever it may
  // occur in the table.  Since we're in a transaction, any changes made by
  // others at the same time will not affect us.
  for (auto const &c : conversions)
    tx.exec(
        "UPDATE pqxxevents "
        "SET year=" +
        pqxx::to_string(c.second) +
        " "
        "WHERE year=" +
        pqxx::to_string(c.first))
      .no_rows();

  tx.commit();

  return conversions;
}


void test_026(pqxx::test::context &)
{
  pqxx::connection cx;
  {
    pqxx::nontransaction tx{cx};
    pqxx::test::create_pqxxevents(tx);
    tx.commit();
  }

  // Perform (an instantiation of) the UpdateYears transactor we've defined
  // in the code above.  This is where the work gets done.
  auto const conversions{pqxx::perform([&cx] { return update_years(cx); })};

  PQXX_CHECK(not std::empty(conversions), "No conversions done!");
}


PQXX_REGISTER_TEST(test_026);
} // namespace
