#include <cmath>
#include <iostream>
#include <sstream>

#include <pqxx/transaction>

#include "test_helpers.hxx"

using namespace pqxx;


// Streams test program for libpqxx.  Insert a result field into various
// types of streams.
namespace
{
void test_046()
{
  connection cx;
  work tx{cx};

  pqxx::field R{tx.exec("SELECT count(*) FROM pg_tables").one_field()};

  // Read the value into a stringstream.
  std::stringstream I;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  I << R;
#include "pqxx/internal/ignore-deprecated-post.hxx"

  // Now convert the stringstream into a numeric type
  long L{}, L2{};
  I >> L;

  R.to(L2);
  PQXX_CHECK_EQUAL(L, L2, "Inconsistency between conversion methods.");

  float F{}, F2{};
  std::stringstream I2;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  I2 << R;
#include "pqxx/internal/ignore-deprecated-post.hxx"
  I2 >> F;
  R.to(F2);
  PQXX_CHECK_BOUNDS(F2, F - 0.01, F + 0.01, "Bad floating-point result.");

  auto F3{from_string<float>(R.c_str())};
  PQXX_CHECK_BOUNDS(F3, F - 0.01, F + 0.01, "Bad float from from_string.");

  auto D{from_string<double>(R.c_str())};
  PQXX_CHECK_BOUNDS(D, F - 0.01, F + 0.01, "Bad double from from_string.");

  auto LD{from_string<long double>(R.c_str())};
  PQXX_CHECK_BOUNDS(
    LD, F - 0.01, F + 0.01, "Bad long double from from_string.");

  auto S{from_string<std::string>(R.c_str())},
    S2{from_string<std::string>(std::string{R.c_str()})},
    S3{from_string<std::string>(R)};

  PQXX_CHECK_EQUAL(
    S2, S,
    "from_string(char const[], std::string &) "
    "is inconsistent with "
    "from_string(std::string const &, std::string &).");

  PQXX_CHECK_EQUAL(
    S3, S2,
    "from_string(result::field const &, std::string &) "
    "is inconsistent with "
    "from_string(std::string const &, std::string &).");

  PQXX_CHECK(tx.query_value<bool>("SELECT 1=1"), "1=1 doesn't yield 'true.'");

  PQXX_CHECK(not tx.query_value<bool>("SELECT 2+2=5"), "2+2=5 yields 'true.'");
}


PQXX_REGISTER_TEST(test_046);
} // namespace
