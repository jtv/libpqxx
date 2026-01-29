#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_field(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const r1{tx.exec("SELECT 9").one_row()};
  auto const &f1{r1[0]};

  PQXX_CHECK_EQUAL(f1.as<std::string>(), "9");
  PQXX_CHECK_EQUAL(f1.as<std::string>("z"), "9");

  PQXX_CHECK_EQUAL(f1.as<int>(), 9);
  PQXX_CHECK_EQUAL(f1.as<int>(10), 9);

  std::string s;
  PQXX_CHECK(f1.to(s));
  PQXX_CHECK_EQUAL(s, "9");
  s = "x";
  PQXX_CHECK(f1.to(s, std::string{"7"}));
  PQXX_CHECK_EQUAL(s, "9");

  int i{};
  PQXX_CHECK(f1.to(i));
  PQXX_CHECK_EQUAL(i, 9);
  i = 8;
  PQXX_CHECK(f1.to(i, 12));
  PQXX_CHECK_EQUAL(i, 9);

  auto const r2{tx.exec("SELECT NULL").one_row()};
  auto const f2{r2[0]};
  i = 100;
  PQXX_CHECK_THROWS(std::ignore = f2.as<int>(), pqxx::conversion_error);
  PQXX_CHECK_EQUAL(i, 100);

  PQXX_CHECK_EQUAL(f2.as<int>(66), 66);

  PQXX_CHECK(!(f2.to(i)));
  PQXX_CHECK(!(f2.to(i, 54)));
  PQXX_CHECK_EQUAL(i, 54);

  auto const r3{tx.exec("SELECT generate_series(1, 5)")};
  PQXX_CHECK_EQUAL(r3.at(3, 0).as<int>(), 4);

  // (Work around totally bogus MSVC warning: it thinks the comma in the C++23
  // two-parameter indexing expression is a sequence operator.  But only for
  // the purposes of issuing annoying warnings; it doesn't _actually_ believe
  // that.)
#if defined(PQXX_HAVE_MULTIDIM)
#  if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4548)
#  endif

  PQXX_CHECK_EQUAL((r3[3, 0].as<int>()), 4);

#  if defined(_MSC_VER)
#    pragma warning(pop)
#  endif

#endif // PQXX_HAVE_MULTIDIM
}


PQXX_REGISTER_TEST(test_field);
} // namespace
