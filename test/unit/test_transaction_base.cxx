#include <pqxx/stream_to>
#include <pqxx/transaction>

#include "../test_helpers.hxx"

namespace
{
void test_exec0(pqxx::transaction_base &trans)
{
  pqxx::result E{trans.exec0("SELECT * FROM pg_tables WHERE 0 = 1")};
  PQXX_CHECK(std::empty(E), "Nonempty result from exec0.");

  PQXX_CHECK_THROWS(
    trans.exec0("SELECT 99"), pqxx::unexpected_rows,
    "Nonempty exec0 result did not throw unexpected_rows.");
}


void test_exec1(pqxx::transaction_base &trans)
{
  pqxx::row R{trans.exec1("SELECT 99")};
  PQXX_CHECK_EQUAL(std::size(R), 1, "Wrong size result from exec1.");
  PQXX_CHECK_EQUAL(R.front().as<int>(), 99, "Wrong result from exec1.");

  PQXX_CHECK_THROWS(
    trans.exec1("SELECT * FROM pg_tables WHERE 0 = 1"), pqxx::unexpected_rows,
    "Empty exec1 result did not throw unexpected_rows.");
  PQXX_CHECK_THROWS(
    trans.exec1("SELECT * FROM generate_series(1, 2)"), pqxx::unexpected_rows,
    "Two-row exec1 result did not throw unexpected_rows.");
}


void test_exec_n(pqxx::transaction_base &trans)
{
  pqxx::result R{trans.exec_n(3, "SELECT * FROM generate_series(1, 3)")};
  PQXX_CHECK_EQUAL(std::size(R), 3, "Wrong result size from exec_n.");

  PQXX_CHECK_THROWS(
    trans.exec_n(2, "SELECT * FROM generate_series(1, 3)"),
    pqxx::unexpected_rows,
    "exec_n did not throw unexpected_rows for an undersized result.");
  PQXX_CHECK_THROWS(
    trans.exec_n(4, "SELECT * FROM generate_series(1, 3)"),
    pqxx::unexpected_rows,
    "exec_n did not throw unexpected_rows for an oversized result.");
}


void test_query_value(pqxx::connection &conn)
{
  pqxx::work tx{conn};

  PQXX_CHECK_EQUAL(
    tx.query_value<int>("SELECT 84 / 2"), 42,
    "Got wrong value from query_value.");
  PQXX_CHECK_THROWS(
    tx.query_value<int>("SAVEPOINT dummy"), pqxx::unexpected_rows,
    "Got field when none expected.");
  PQXX_CHECK_THROWS(
    tx.query_value<int>("SELECT generate_series(1, 2)"), pqxx::unexpected_rows,
    "Failed to fail for multiple rows.");
  PQXX_CHECK_THROWS(
    tx.query_value<int>("SELECT 1, 2"), pqxx::usage_error,
    "No error for too many fields.");
  PQXX_CHECK_THROWS(
    tx.query_value<int>("SELECT 3.141"), pqxx::conversion_error,
    "Got int field from float string.");
}


void test_transaction_base()
{
  pqxx::connection conn;
  {
    pqxx::work tx{conn};
    test_exec_n(tx);
    test_exec0(tx);
    test_exec1(tx);
  }
  test_query_value(conn);
}


void test_transaction_query()
{
  pqxx::connection c;
  pqxx::work tx{c};

  std::vector<std::string> names;
  std::vector<int> salaries;

  for (auto [name, salary] : tx.query<std::string, int>(
         "SELECT 'name' || i, i * 1000 FROM generate_series(1, 5) AS i"))
  {
    names.emplace_back(name);
    salaries.emplace_back(salary);
  }

  PQXX_CHECK_EQUAL(std::size(names), 5u, "Wrong number of rows.");
  PQXX_CHECK_EQUAL(std::size(salaries), 5u, "Mismatched number of salaries!");
  PQXX_CHECK_EQUAL(names[0], "name1", "Names start out wrong.");
  PQXX_CHECK_EQUAL(names[4], "name5", "Names end wrong.");
  PQXX_CHECK_EQUAL(salaries[0], 1'000, "Salaries start out wrong.");
  PQXX_CHECK_EQUAL(salaries[4], 5'000, "Salaries end wrong.");
}


void test_transaction_for_query()
{
  constexpr auto query{
    "SELECT i, concat('x', (2*i)::text) "
    "FROM generate_series(1, 3) AS i "
    "ORDER BY i"};
  pqxx::connection conn;
  pqxx::work tx{conn};
  std::string ints;
  std::string strings;
  tx.for_query(query, [&ints, &strings](int i, std::string const &s) {
    ints += pqxx::to_string(i) + " ";
    strings += s + " ";
  });
  PQXX_CHECK_EQUAL(ints, "1 2 3 ", "Unexpected int sequence.");
  PQXX_CHECK_EQUAL(strings, "x2 x4 x6 ", "Unexpected string sequence.");
}


void test_transaction_for_stream()
{
  constexpr auto query{
    "SELECT i, concat('x', (2*i)::text) "
    "FROM generate_series(1, 3) AS i "
    "ORDER BY i"};
  pqxx::connection conn;
  pqxx::work tx{conn};
  std::string ints;
  std::string strings;
  tx.for_stream(query, [&ints, &strings](int i, std::string const &s) {
    ints += pqxx::to_string(i) + " ";
    strings += s + " ";
  });
  PQXX_CHECK_EQUAL(ints, "1 2 3 ", "Unexpected int sequence.");
  PQXX_CHECK_EQUAL(strings, "x2 x4 x6 ", "Unexpected string sequence.");
}


void test_transaction_query01()
{
  pqxx::connection c;
  pqxx::work w{c};

  std::optional<std::tuple<int>> o{
    w.query01<int>("SELECT * FROM generate_series(1, 1) AS i WHERE i = 5")};
  PQXX_CHECK(not o.has_value(), "Why did we get a row?");
  o = w.query01<int>("SELECT * FROM generate_series(8, 8)");
  PQXX_CHECK(o.has_value(), "Why did we not get a row?");

  // The "if" is redundant (see the check above).  But gcc 11 complains when
  // enabling maintainer mode but not audit mode: something inside the optional
  // "may be used uninitialized."
  if (o.has_value())
    PQXX_CHECK_EQUAL(std::get<0>(*o), 8, "Bad value from query01().");
  PQXX_CHECK_THROWS(
    o = w.query01<int>("SELECT * FROM generate_series(1, 2)"),
    pqxx::unexpected_rows,
    "Wrong number of rows did not throw unexpected_rows.");
  PQXX_CHECK_THROWS(
    o = w.query01<int>("SELECT 1, 2"), pqxx::usage_error,
    "Wrong number of columns did not throw usage_error.");
}


void test_transaction_query1()
{
  pqxx::connection c;
  pqxx::work w{c};

  std::tuple<int> x;
  PQXX_CHECK_THROWS(
    x = w.query1<int>("SELECT * FROM generate_series(1, 1) AS i WHERE i = 5"),
    pqxx::unexpected_rows, "Zero rows did not throw unexpected_rows.");
  std::optional<std::tuple<int>> const o{
    w.query1<int>("SELECT * FROM generate_series(8, 8)")};
  PQXX_CHECK(o.has_value(), "Why did we not get a row?");
  PQXX_CHECK_EQUAL(std::get<0>(*o), 8, "Bad value from query1().");
  PQXX_CHECK_THROWS(
    x = w.query1<int>("SELECT * FROM generate_series(1, 2)"),
    pqxx::unexpected_rows, "Too many rows did not throw unexpected_rows.");
  PQXX_CHECK_THROWS(
    x = w.query1<int>("SELECT 1, 2"), pqxx::usage_error,
    "Wrong number of columns did not throw usage_error.");
  pqxx::ignore_unused(x);
}


void test_transaction_query_n()
{
  pqxx::connection c;
  pqxx::work w{c};

  PQXX_CHECK_THROWS(
    pqxx::ignore_unused(w.query_n<int>(5, "SELECT generate_series(1, 3)")),
    pqxx::unexpected_rows, "No exception when query_n returns too few rows.");
  PQXX_CHECK_THROWS(
    pqxx::ignore_unused(w.query_n<int>(5, "SELECT generate_series(1, 10)")),
    pqxx::unexpected_rows, "No exception when query_n returns too many rows.");

  std::vector<int> v;
  for (auto [n] : w.query_n<int>(3, "SELECT generate_series(7, 9)"))
    v.push_back(n);
  PQXX_CHECK_EQUAL(std::size(v), 3u, "Wrong number of rows.");
  PQXX_CHECK_EQUAL(v[0], 7, "Wrong result data.");
  PQXX_CHECK_EQUAL(v[2], 9, "Data started out right but went wrong.");
}


PQXX_REGISTER_TEST(test_transaction_base);
PQXX_REGISTER_TEST(test_transaction_query);
PQXX_REGISTER_TEST(test_transaction_for_query);
PQXX_REGISTER_TEST(test_transaction_for_stream);
PQXX_REGISTER_TEST(test_transaction_query01);
PQXX_REGISTER_TEST(test_transaction_query1);
PQXX_REGISTER_TEST(test_transaction_query_n);
} // namespace
