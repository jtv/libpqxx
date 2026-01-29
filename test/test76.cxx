#include <pqxx/nontransaction>

#include "helpers.hxx"

// Simple test program for libpqxx.  Test string conversion routines.
namespace
{
void test_076(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::nontransaction tx{cx};

  auto RFalse{tx.exec("SELECT 1=0").one_field()},
    RTrue{tx.exec("SELECT 1=1").one_field()};
  auto False{pqxx::from_string<bool>(RFalse)},
    True{pqxx::from_string<bool>(RTrue)};
  PQXX_CHECK(not False);
  PQXX_CHECK(True);

  RFalse = tx.exec("SELECT " + pqxx::to_string(False)).one_field();
  RTrue = tx.exec("SELECT " + pqxx::to_string(True)).one_field();
  False = pqxx::from_string<bool>(RFalse);
  True = pqxx::from_string<bool>(RTrue);
  PQXX_CHECK(not False);
  PQXX_CHECK(True);

  constexpr std::array<short, 7> svals{-1, 1, 999, -32767, -32768, 32767, 0};
  for (short const v : svals)
  {
    auto s{pqxx::from_string<short>(pqxx::to_string(v))};
    PQXX_CHECK_EQUAL(s, v);
    s = pqxx::from_string<short>(
      tx.exec("SELECT " + pqxx::to_string(v)).one_field().c_str());
    PQXX_CHECK_EQUAL(s, v);
  }

  constexpr std::array<unsigned short, 6> uvals{1,     999,   32767,
                                                32768, 65535, 0};
  for (unsigned short const v : uvals)
  {
    auto u{pqxx::from_string<unsigned short>(pqxx::to_string(v))};
    PQXX_CHECK_EQUAL(u, v);

    u = pqxx::from_string<unsigned short>(
      tx.exec("SELECT " + pqxx::to_string(v)).one_field().c_str());
    PQXX_CHECK_EQUAL(u, v);
  }
}
} // namespace


PQXX_REGISTER_TEST(test_076);
