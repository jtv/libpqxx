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
  bool
	False = from_string<bool>(RFalse[0]),
	True = from_string<bool>(RTrue[0]);
  PQXX_CHECK(not False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  RFalse = tx.exec1("SELECT " + to_string(False));
  RTrue  = tx.exec1("SELECT " + to_string(True));
  False = from_string<bool>(RFalse[0]);
  True = from_string<bool>(RTrue[0]);
  PQXX_CHECK(not False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  const short svals[] = { -1, 1, 999, -32767, -32768, 32767, 0 };
  for (int i=0; svals[i]; ++i)
  {
    short s = from_string<short>(to_string(svals[i]));
    PQXX_CHECK_EQUAL(s, svals[i], "short/string conversion not bijective.");
    s = from_string<short>(
	tx.exec1("SELECT " + to_string(svals[i]))[0].c_str());
    PQXX_CHECK_EQUAL(s, svals[i], "Roundtrip through backend changed short.");
  }

  const unsigned short uvals[] = { 1, 999, 32767, 32768, 65535, 0 };
  for (int i=0; uvals[i]; ++i)
  {
    unsigned short u = from_string<unsigned short>(to_string(uvals[i]));
    PQXX_CHECK_EQUAL(
	u,
	uvals[i],
	"unsigned short/string conversion not bijective.");

    u = from_string<unsigned short>(
	tx.exec1("SELECT " + to_string(uvals[i]))[0].c_str());
    PQXX_CHECK_EQUAL(
	u,
	uvals[i],
	"Roundtrip through backend changed unsigned short.");
  }
}
} // namespace


PQXX_REGISTER_TEST(test_076);
