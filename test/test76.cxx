#include <pqxx/nontransaction>

#include "test_helpers.hxx"

// Simple test program for libpqxx.  Test string conversion routines.
namespace
{
void test_076()
{
  pqxx::connection cx;
  pqxx::nontransaction tx{cx};

  auto RFalse{tx.exec("SELECT 1=0").one_field()},
    RTrue{tx.exec("SELECT 1=1").one_field()};
  auto False{pqxx::from_string<bool>(RFalse)},
    True{pqxx::from_string<bool>(RTrue)};
  PQXX_CHECK(not False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  RFalse = tx.exec("SELECT " + pqxx::to_string(False)).one_field();
  RTrue = tx.exec("SELECT " + pqxx::to_string(True)).one_field();
  False = pqxx::from_string<bool>(RFalse);
  True = pqxx::from_string<bool>(RTrue);
  PQXX_CHECK(not False, "False bool converted to true.");
  PQXX_CHECK(True, "True bool converted to false.");

  short const svals[]{-1, 1, 999, -32767, -32768, 32767, 0};
  for (int i{0}; svals[i] != 0; ++i)
  {
    auto s{pqxx::from_string<short>(pqxx::to_string(svals[i]))};
    PQXX_CHECK_EQUAL(s, svals[i], "short/string conversion not bijective.");
    s = pqxx::from_string<short>(
      tx.exec("SELECT " + pqxx::to_string(svals[i])).one_field().c_str());
    PQXX_CHECK_EQUAL(s, svals[i], "Roundtrip through backend changed short.");
  }

  unsigned short const uvals[]{1, 999, 32767, 32768, 65535, 0};
  for (int i{0}; uvals[i] != 0; ++i)
  {
    auto u{pqxx::from_string<unsigned short>(pqxx::to_string(uvals[i]))};
    PQXX_CHECK_EQUAL(
      u, uvals[i], "unsigned short/string conversion not bijective.");

    u = pqxx::from_string<unsigned short>(
      tx.exec("SELECT " + pqxx::to_string(uvals[i])).one_field().c_str());
    PQXX_CHECK_EQUAL(
      u, uvals[i], "Roundtrip through backend changed unsigned short.");
  }
}
} // namespace


PQXX_REGISTER_TEST(test_076);
