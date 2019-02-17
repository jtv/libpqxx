#include "test_helpers.hxx"

using namespace pqxx;


// Simple test program for libpqxx.  Test string conversion routines.
namespace
{
void test_076()
{
  connection conn;
  nontransaction tx{conn};

  row
	RFalse = tx.exec1("SELECT 1=0"),
	RTrue  = tx.exec1("SELECT 1=1");
  bool False, True;
  from_string(RFalse[0], False);
  from_string(RTrue[0],  True);
  PQXX_CHECK(not False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  RFalse = tx.exec1("SELECT " + to_string(False));
  RTrue  = tx.exec1("SELECT " + to_string(True));
  from_string(RFalse[0], False);
  from_string(RTrue[0],  True);
  PQXX_CHECK(not False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  const short svals[] = { -1, 1, 999, -32767, -32768, 32767, 0 };
  for (int i=0; svals[i]; ++i)
  {
    short s;
    from_string(to_string(svals[i]), s);
    PQXX_CHECK_EQUAL(s, svals[i], "short/string conversion not bijective.");
    from_string(tx.exec("SELECT " + to_string(svals[i]))[0][0].c_str(), s);
    PQXX_CHECK_EQUAL(s, svals[i], "Roundtrip through backend changed short.");
  }

  const unsigned short uvals[] = { 1, 999, 32767, 32768, 65535, 0 };
  for (int i=0; uvals[i]; ++i)
  {
    unsigned short u;
    from_string(to_string(uvals[i]), u);
    PQXX_CHECK_EQUAL(
	u,
	uvals[i],
	"unsigned short/string conversion not bijective.");

    from_string(tx.exec("SELECT " + to_string(uvals[i]))[0][0].c_str(), u);
    PQXX_CHECK_EQUAL(
	u,
	uvals[i],
	"Roundtrip through backend changed unsigned short.");
  }
}
} // namespace


PQXX_REGISTER_TEST(test_076);
