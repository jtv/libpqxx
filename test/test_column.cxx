#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_table_column(pqxx::test::context &)
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  tx.exec("CREATE TEMP TABLE pqxxfoo (x varchar, y integer, z integer)")
    .no_rows();
  tx.exec("INSERT INTO pqxxfoo VALUES ('xx', 1, 2)").no_rows();
  auto R{tx.exec("SELECT z,y,x FROM pqxxfoo")};
  auto X{tx.exec("SELECT x,y,z,99 FROM pqxxfoo")};

  pqxx::row::size_type x{R.table_column(2)}, y{R.table_column(1)},
    z{R.table_column(static_cast<int>(0))};

  PQXX_CHECK_EQUAL(x, 0);
  PQXX_CHECK_EQUAL(y, 1);
  PQXX_CHECK_EQUAL(z, 2);

  x = R.table_column("x");
  y = R.table_column("y");
  z = R.table_column("z");

  PQXX_CHECK_EQUAL(x, 0);
  PQXX_CHECK_EQUAL(y, 1);
  PQXX_CHECK_EQUAL(z, 2);

  pqxx::row::size_type const xx{X[0].table_column(static_cast<int>(0))},
    yx{X[0].table_column(pqxx::row::size_type(1))}, zx{X[0].table_column("z")};

  PQXX_CHECK_EQUAL(xx, 0, "Bad result from table_column(int).");
  PQXX_CHECK_EQUAL(yx, 1, "Bad result from table_column(size_type).");
  PQXX_CHECK_EQUAL(zx, 2, "Bad result from table_column(string).");

  for (pqxx::row::size_type i{0}; i < std::size(R[0]); ++i)
    PQXX_CHECK_EQUAL(R[0][i].table_column(), R.table_column(i));

  [[maybe_unused]] int col{};
  PQXX_CHECK_THROWS_EXCEPTION(col = R.table_column(3));

  PQXX_CHECK_THROWS_EXCEPTION(col = R.table_column("nonexistent"));

  PQXX_CHECK_THROWS_EXCEPTION(col = X.table_column(3));
}
} // namespace


PQXX_REGISTER_TEST(test_table_column);
