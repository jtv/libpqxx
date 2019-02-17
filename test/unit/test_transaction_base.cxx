#include "../test_helpers.hxx"

using namespace pqxx;

namespace
{
void test_exec0(transaction_base &trans)
{
  result E = trans.exec0("SELECT * FROM pg_tables WHERE 0 = 1");
  PQXX_CHECK(E.empty(), "Nonempty result from exec0.");

  PQXX_CHECK_THROWS(
	trans.exec0("SELECT 99"),
	unexpected_rows,
	"Nonempty exec0 result did not throw unexpected_rows.");
}


void test_exec1(transaction_base &trans)
{
  row R = trans.exec1("SELECT 99");
  PQXX_CHECK_EQUAL(R.size(), size_t(1), "Wrong size result from exec1.");
  PQXX_CHECK_EQUAL(R.front().as<int>(), 99, "Wrong result from exec1.");

  PQXX_CHECK_THROWS(
	trans.exec1("SELECT * FROM pg_tables WHERE 0 = 1"),
	unexpected_rows,
	"Empty exec1 result did not throw unexpected_rows.");
  PQXX_CHECK_THROWS(
	trans.exec1("SELECT * FROM generate_series(1, 2)"),
	unexpected_rows,
	"Two-row exec1 result did not throw unexpected_rows.");
}


void test_exec_n(transaction_base &trans)
{
  result R = trans.exec_n(3, "SELECT * FROM generate_series(1, 3)");
  PQXX_CHECK_EQUAL(R.size(), size_t(3), "Wrong result size from exec_n.");

  PQXX_CHECK_THROWS(
	trans.exec_n(2, "SELECT * FROM generate_series(1, 3)"),
	unexpected_rows,
	"exec_n did not throw unexpected_rows for an undersized result.");
  PQXX_CHECK_THROWS(
	trans.exec_n(4, "SELECT * FROM generate_series(1, 3)"),
	unexpected_rows,
	"exec_n did not throw unexpected_rows for an oversized result.");
}


void test_transaction_base()
{
  connection conn;
  work tx{conn};
  test_exec_n(tx);
  test_exec0(tx);
  test_exec1(tx);
}


PQXX_REGISTER_TEST(test_transaction_base);
} // namespace
