#include <cmath>
#include <iostream>
#include <sstream>

#include "test_helpers.hxx"

using namespace pqxx;


// Streams test program for libpqxx.  Insert a result field into various
// types of streams.
namespace
{
void test_046()
{
  connection conn;
  work tx{conn};

  row R{ tx.exec1("SELECT count(*) FROM pg_tables") };

  std::cout << "Count was " << R.front() << std::endl;

  // Read the value into a stringstream
  std::stringstream I;
  I << R[0];

  // Now convert the stringstream into a numeric type
  long L, L2;
  I >> L;
  std::cout << "As a long, it's " << L << std::endl;

  R[0].to(L2);
  PQXX_CHECK_EQUAL(L, L2, "Inconsistency between conversion methods.");

  float F, F2;
  std::stringstream I2;
  I2 << R[0];
  I2 >> F;
  std::cout << "As a float, it's " << F << std::endl;
  R[0].to(F2);
  PQXX_CHECK_BOUNDS(F2, F-0.01, F+0.01, "Bad floating-point result.");

  float F3;
  from_string(R[0].c_str(), F3);
  PQXX_CHECK_BOUNDS(F3, F-0.01, F+0.01, "Bad float from from_string.");

  double D;
  from_string(R[0].c_str(), D);
  PQXX_CHECK_BOUNDS(D, F-0.01, F+0.01, "Bad double from from_string.");

  long double LD;
  from_string(R[0].c_str(), LD);
  PQXX_CHECK_BOUNDS(LD, F-0.01, F+0.01, "Bad long double from from_string.");

  std::string S, S2, S3;
  from_string(R[0].c_str(), S);
  from_string(std::string{R[0].c_str()}, S2);
  from_string(R[0], S3);

  PQXX_CHECK_EQUAL(
	S2,
	S,
	"from_string(const char[], std::string &) "
	"is inconsistent with "
	"from_string(const std::string &, std::string &).");

  PQXX_CHECK_EQUAL(
	S3,
	S2,
	"from_string(const result::field &, std::string &) "
	"is inconsistent with "
	"from _string(const std::string &, std::string &).");

  PQXX_CHECK(
	tx.exec1("SELECT 1=1").front().as<bool>(),
	"1=1 doesn't yield 'true.'");

  PQXX_CHECK(
	not tx.exec1("SELECT 2+2=5").front().as<bool>(),
	"2+2=5 yields 'true.'");
}


PQXX_REGISTER_TEST(test_046);
} // namespace
