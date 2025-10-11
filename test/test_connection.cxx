#include <numeric>

#include <pqxx/nontransaction>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_connection_string_constructor()
{
  pqxx::connection const c1{""};
  pqxx::connection const c2{std::string{}};
}


void test_move_constructor()
{
  pqxx::connection c1;
  PQXX_CHECK(c1.is_open());

  pqxx::connection c2{std::move(c1)};

  // Checking for predictable state of an object after its state has been
  // moved out...  clang-tidy doesn't like this.
  PQXX_CHECK(not c1.is_open()); // NOLINT
  PQXX_CHECK(c2.is_open());

  pqxx::work tx{c2};
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT 5"), 5);

  PQXX_CHECK_THROWS(pqxx::connection{std::move(c2)}, pqxx::usage_error);
}


void test_move_assign()
{
  pqxx::connection c1;
  pqxx::connection c2;

  c2.close();

  c2 = std::move(c1);

  PQXX_CHECK(not c1.is_open()); // NOLINT
  PQXX_CHECK(c2.is_open());

  {
    pqxx::work tx1{c2};
    PQXX_CHECK_EQUAL(tx1.query_value<int>("SELECT 8"), 8, "What!?");

    pqxx::connection c3;
    PQXX_CHECK_THROWS(c1 = std::move(c2), pqxx::usage_error);
    PQXX_CHECK_THROWS(c2 = std::move(c3), pqxx::usage_error);
  }

  // After failed move attempts, the connection is still usable.
  pqxx::work tx2{c2};
  PQXX_CHECK_EQUAL(tx2.query_value<int>("SELECT 6"), 6, "Huh!?");
}


void test_encrypt_password()
{
  pqxx::connection c;
  auto pw{c.encrypt_password("user", "password")};
  PQXX_CHECK(not std::empty(pw));
  PQXX_CHECK_EQUAL(
    std::strlen(pw.c_str()), std::size(pw),
    "Encrypted password contains a null byte.");
}


void test_connection_string()
{
  pqxx::connection const c;
  std::string const connstr{c.connection_string()};

#if defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
  if (std::getenv("PGUSER") == nullptr)
#if defined(_MSC_VER)
#  pragma warning(pop)
#endif
  {
    PQXX_CHECK(
      pqxx::str_contains(connstr, "user=" + std::string{c.username()}),
      "Connection string did not specify user name: " + connstr);
  }
  else
  {
    PQXX_CHECK(
      not pqxx::str_contains(connstr, "user=" + std::string{c.username()}),
      "Connection string specified user name, even when using default: " +
        connstr);
  }
}


template<typename STR> std::size_t length(STR const &str)
{
  return std::size(str);
}


std::size_t length(char const str[])
{
  return std::strlen(str);
}


template<typename MAP> void test_params_type()
{
  using item_t = std::remove_reference_t<
    decltype(*std::declval<std::ranges::iterator_t<MAP>>())>;
  using key_t = decltype(std::get<0>(std::declval<item_t>()));
  using value_t = decltype(std::get<1>(std::declval<item_t>()));

  // Set some parameters that are relatively safe to change arbitrarily.
  MAP const params{{
    {key_t{"application_name"}, value_t{"pqxx-test"}},
    {key_t{"connect_timeout"}, value_t{"96"}},
    {key_t{"keepalives_idle"}, value_t{"771"}},
  }};

  // Can we create a connection from these parameters?
  pqxx::connection const c{params};

  // Check that the parameters came through in the connection string.
  // We don't know the exact format, but the parameters have to be in there.
  auto const min_size{std::accumulate(
    std::cbegin(params), std::cend(params), std::size(params) - 1,
    [](auto count, auto item) {
      return count + length(std::get<0>(item)) + 1 + length(std::get<1>(item));
    })};

  auto const connstr{c.connection_string()};
  PQXX_CHECK_GREATER_EQUAL(
    std::size(connstr), min_size,
    "Connection string can't possibly contain the options we gave.");
  for (auto const &[key, value] : params)
  {
    PQXX_CHECK(
      pqxx::str_contains(connstr, key),
      "Could not find param name '" + std::string{key} +
        "' in connection string: " + connstr);
    PQXX_CHECK(
      pqxx::str_contains(connstr, value),
      "Could not find value for '" + std::string{value} +
        "' in connection string: " + connstr);
  }
}


void test_connection_params()
{
  // Connecting in this way supports a wide variety of formats for the
  // parameters.
  test_params_type<std::map<char const *, char const *>>();
  test_params_type<std::map<pqxx::zview, pqxx::zview>>();
  test_params_type<std::map<std::string, std::string>>();
  test_params_type<std::map<std::string, pqxx::zview>>();
  test_params_type<std::map<pqxx::zview, char const *>>();
  test_params_type<std::vector<std::tuple<char const *, char const *>>>();
  test_params_type<std::vector<std::tuple<pqxx::zview, std::string>>>();
  test_params_type<std::vector<std::pair<std::string, char const *>>>();
  test_params_type<std::vector<std::array<std::string, 2>>>();
}


void test_raw_connection()
{
  pqxx::connection conn1;
  PQXX_CHECK(conn1.is_open());
  pqxx::nontransaction tx1{conn1};
  PQXX_CHECK_EQUAL(tx1.query_value<int>("SELECT 8"), 8);

  // Checking for predictable bewhaviour of a dead object after move.
  // clang-tidy doesn't like this.
  pqxx::internal::pq::PGconn *raw{std::move(conn1).release_raw_connection()};
  PQXX_CHECK(raw != nullptr);      // NOLINT
  PQXX_CHECK(not conn1.is_open()); // NOLINT

  pqxx::connection conn2{pqxx::connection::seize_raw_connection(raw)};
  PQXX_CHECK(conn2.is_open());
  pqxx::nontransaction tx2{conn2};
  PQXX_CHECK_EQUAL(tx2.query_value<int>("SELECT 9"), 9);
}


void test_closed_connection()
{
  pqxx::connection cx;
  cx.close();
  PQXX_CHECK(not cx.dbname());
  PQXX_CHECK(not cx.username());
  PQXX_CHECK(not cx.hostname());
  PQXX_CHECK(not cx.port());
}


void test_skip_init_ssl()
{
  pqxx::skip_init_ssl<pqxx::skip_init::openssl, pqxx::skip_init::crypto>();
  pqxx::skip_init_ssl<pqxx::skip_init::nothing>();
}


PQXX_REGISTER_TEST(test_connection_string_constructor);
PQXX_REGISTER_TEST(test_move_constructor);
PQXX_REGISTER_TEST(test_move_assign);
PQXX_REGISTER_TEST(test_encrypt_password);
PQXX_REGISTER_TEST(test_connection_string);
PQXX_REGISTER_TEST(test_connection_params);
PQXX_REGISTER_TEST(test_raw_connection);
PQXX_REGISTER_TEST(test_closed_connection);
PQXX_REGISTER_TEST(test_skip_init_ssl);
} // namespace
