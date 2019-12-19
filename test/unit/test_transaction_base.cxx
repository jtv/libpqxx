#include "../test_helpers.hxx"

namespace
{
void test_exec0(pqxx::transaction_base &trans)
{
  pqxx::result E = trans.exec0("SELECT * FROM pg_tables WHERE 0 = 1");
  PQXX_CHECK(E.empty(), "Nonempty result from exec0.");

  PQXX_CHECK_THROWS(
	trans.exec0("SELECT 99"),
	pqxx::unexpected_rows,
	"Nonempty exec0 result did not throw unexpected_rows.");
}


void test_exec1(pqxx::transaction_base &trans)
{
  pqxx::row R = trans.exec1("SELECT 99");
  PQXX_CHECK_EQUAL(R.size(), 1, "Wrong size result from exec1.");
  PQXX_CHECK_EQUAL(R.front().as<int>(), 99, "Wrong result from exec1.");

  PQXX_CHECK_THROWS(
	trans.exec1("SELECT * FROM pg_tables WHERE 0 = 1"),
	pqxx::unexpected_rows,
	"Empty exec1 result did not throw unexpected_rows.");
  PQXX_CHECK_THROWS(
	trans.exec1("SELECT * FROM generate_series(1, 2)"),
	pqxx::unexpected_rows,
	"Two-row exec1 result did not throw unexpected_rows.");
}


void test_exec_n(pqxx::transaction_base &trans)
{
  pqxx::result R = trans.exec_n(3, "SELECT * FROM generate_series(1, 3)");
  PQXX_CHECK_EQUAL(R.size(), 3, "Wrong result size from exec_n.");

  PQXX_CHECK_THROWS(
	trans.exec_n(2, "SELECT * FROM generate_series(1, 3)"),
	pqxx::unexpected_rows,
	"exec_n did not throw unexpected_rows for an undersized result.");
  PQXX_CHECK_THROWS(
	trans.exec_n(4, "SELECT * FROM generate_series(1, 3)"),
	pqxx::unexpected_rows,
	"exec_n did not throw unexpected_rows for an oversized result.");
}


void test_transaction_base()
{
  pqxx::connection conn;
  pqxx::work tx{conn};
  test_exec_n(tx);
  test_exec0(tx);
  test_exec1(tx);
}


PQXX_REGISTER_TEST(test_transaction_base);
} // namespace
