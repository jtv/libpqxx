#include "test_helpers.hxx"

using namespace std;
using namespace pqxx;


// Simple test program for libpqxx.  Test string conversion routines.
namespace
{
void test_076(transaction_base &T)
{
  result RFalse = T.exec("SELECT 1=0"),
	 RTrue  = T.exec("SELECT 1=1");
  bool False, True;
  from_string(RFalse[0][0], False);
  from_string(RTrue[0][0],  True);
  PQXX_CHECK(!False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  RFalse = T.exec("SELECT " + to_string(False));
  RTrue  = T.exec("SELECT " + to_string(True));
  from_string(RFalse[0][0], False);
  from_string(RTrue[0][0],  True);
  PQXX_CHECK(!False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  const short svals[] = { -1, 1, 999, -32767, -32768, 32767, 0 };
  for (int i=0; svals[i]; ++i)
  {
    short s;
    from_string(to_string(svals[i]), s);
    PQXX_CHECK_EQUAL(s, svals[i], "short/string conversion not bijective.");
    from_string(T.exec("SELECT " + to_string(svals[i]))[0][0].c_str(), s);
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

    from_string(T.exec("SELECT " + to_string(uvals[i]))[0][0].c_str(), u);
    PQXX_CHECK_EQUAL(
	u,
	uvals[i],
	"Roundtrip through backend changed unsigned short.");
  }
}
} // namespace

PQXX_REGISTER_TEST_T(test_076, nontransaction)
