#include <functional>
#include <iostream>
#include <map>

#include "test_helpers.hxx"

using namespace pqxx;


// Example program for libpqxx.  Modify the database, retaining transactional
// integrity using the transactor framework, and using lazy connections.
namespace
{
// Convert year to 4-digit format.
int To4Digits(int Y)
{
  int Result = Y;

  PQXX_CHECK(Y >= 0, "Negative year: " + to_string(Y));

  if (Y  < 70)   Result += 2000;
  else if (Y  < 100)  Result += 1900;
  else PQXX_CHECK(Y >= 1970, "Unexpected year: " + to_string(Y));
  return Result;
}


// Transaction definition for year-field update.  Returns conversions done.
std::map<int, int> update_years(connection_base &C)
{
  std::map<int, int> conversions;
  work tx{C};

  // First select all different years occurring in the table.
  result R{ tx.exec("SELECT year FROM pqxxevents") };

  // Note all different years currently occurring in the table, writing them
  // and their correct mappings to m_conversions
  for (const auto &r: R)
  {
    int Y;

    // Read year, and if it is non-null, note its converted value
    if (r[0].to(Y)) conversions[Y] = To4Digits(Y);
  }

  // For each occurring year, write converted date back to whereever it may
  // occur in the table.  Since we're in a transaction, any changes made by
  // others at the same time will not affect us.
  for (const auto &c: conversions)
    tx.exec0(
	"UPDATE pqxxevents "
	"SET year=" + to_string(c.second) + " "
	"WHERE year=" + to_string(c.first));

  tx.commit();

  return conversions;
}


void test_026()
{
  lazyconnection conn;
  {
    nontransaction tx{conn};
    test::create_pqxxevents(tx);
    tx.commit();
  }

  // Perform (an instantiation of) the UpdateYears transactor we've defined
  // in the code above.  This is where the work gets done.
  const auto conversions = perform(std::bind(update_years, std::ref(conn)));

  // Just for fun, report the exact conversions performed.  Note that this
  // list will be accurate even if other people were modifying the database
  // at the same time; this property was established through use of the
  // transactor framework.
  for (const auto &i: conversions)
    std::cout << '\t' << i.first << "\t-> " << i.second << std::endl;
}


PQXX_REGISTER_TEST(test_026);
} // namespace
