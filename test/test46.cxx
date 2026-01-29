#include <cmath>
#include <sstream>

#include <pqxx/transaction>

#include "helpers.hxx"


// Streams test program for libpqxx.  Insert a result field into various
// types of streams.
namespace
{
void test_046(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  pqxx::field const r{tx.exec("SELECT count(*) FROM pg_tables").one_field()};

  // Read the value into a stringstream.
  std::stringstream i;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  i << r;
#include "pqxx/internal/ignore-deprecated-post.hxx"

  // Now convert the stringstream into a numeric type
  long L{}, L2{};
  i >> L;

  r.to(L2);
  PQXX_CHECK_EQUAL(L, L2, "Inconsistency between conversion methods.");

  float f{}, f2{};
  std::stringstream I2;
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  I2 << r;
#include "pqxx/internal/ignore-deprecated-post.hxx"
  I2 >> f;
  r.to(f2);
  PQXX_CHECK_BOUNDS(f2, f - 0.01, f + 0.01);

  auto F3{pqxx::from_string<float>(r.c_str())};
  PQXX_CHECK_BOUNDS(F3, f - 0.01, f + 0.01);

  auto D{pqxx::from_string<double>(r.c_str())};
  PQXX_CHECK_BOUNDS(D, f - 0.01, f + 0.01);

  // Valgrind doesn't support "long double."
#if !defined(PQXX_VALGRIND)
  auto LD{pqxx::from_string<long double>(r.c_str())};
  PQXX_CHECK_BOUNDS(LD, f - 0.01, f + 0.01);
#endif

  auto S{pqxx::from_string<std::string>(r.c_str())},
    S2{pqxx::from_string<std::string>(std::string{r.c_str()})},
    S3{pqxx::from_string<std::string>(r)};

  PQXX_CHECK_EQUAL(S2, S);

  PQXX_CHECK_EQUAL(S3, S2);

  PQXX_CHECK(tx.query_value<bool>("SELECT 1=1"));

  PQXX_CHECK(not tx.query_value<bool>("SELECT 2+2=5"));
}


PQXX_REGISTER_TEST(test_046);
} // namespace
