#include <cmath>

#include <pqxx/transaction>

#include "helpers.hxx"


// Test program for libpqxx.  Test fieldstream.
namespace
{
void test_074(pqxx::test::context &)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::connection cx;
  pqxx::work tx{cx};

  pqxx::result R{tx.exec("SELECT * FROM pg_tables")};
  std::string const sval{R.at(0).at(1).c_str()};
  std::string sval2;
  pqxx::fieldstream fs1(R.front()[1]);
  fs1 >> sval2;
  PQXX_CHECK_EQUAL(sval2, sval, "fieldstream returned wrong value.");

  R = tx.exec("SELECT count(*) FROM pg_tables");
  int ival{};
  pqxx::fieldstream fs2(R.at(0).at(0));
  fs2 >> ival;
  PQXX_CHECK_EQUAL(ival, R.front().front().as<int>());

  double dval{};
  (pqxx::fieldstream(R.at(0).at(0))) >> dval;
  PQXX_CHECK_BOUNDS(
    dval, R[0][0].as<double>() - 0.1, R[0][0].as<double>() + 0.1);

  auto const roughpi{static_cast<float>(3.1415926435)};
  R = tx.exec("SELECT " + pqxx::to_string(roughpi));
  float pival{};
  (pqxx::fieldstream(R.at(0).at(0))) >> pival;
  PQXX_CHECK_BOUNDS(pival, roughpi - 0.001, roughpi + 0.001);

  PQXX_CHECK_EQUAL(pqxx::to_string(R[0][0]), R[0][0].c_str());

  float float_pi{};
  pqxx::from_string(pqxx::to_string(roughpi), float_pi);
  PQXX_CHECK_BOUNDS(float_pi, roughpi - 0.00001, roughpi + 0.00001);

  double double_pi{};
  pqxx::from_string(pqxx::to_string(static_cast<double>(roughpi)), double_pi);
  PQXX_CHECK_BOUNDS(double_pi, roughpi - 0.00001, roughpi + 0.00001);

  // Valgrind doesn't support "long double."
#if !defined(PQXX_VALGRIND)
  long double const ld{roughpi};
  long double long_double_pi{};
  pqxx::from_string(pqxx::to_string(ld), long_double_pi);
  PQXX_CHECK_BOUNDS(long_double_pi, roughpi - 0.00001, roughpi + 0.00001);
#endif

#include "pqxx/internal/ignore-deprecated-post.hxx"
}
} // namespace


PQXX_REGISTER_TEST(test_074);
