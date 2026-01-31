#include <numeric>

#include <pqxx/nontransaction>
#include <pqxx/transaction>

#include "helpers.hxx"

namespace
{
void test_connection_string_constructor(pqxx::test::context &)
{
  pqxx::connection const c1{""};
  PQXX_CHECK(c1.is_open());
  pqxx::connection const c2{std::string{}};
  PQXX_CHECK(c2.is_open());
  pqxx::connection const c3{pqxx::zview{""}};
  PQXX_CHECK(c3.is_open());
}


void test_move_constructor(pqxx::test::context &)
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


void test_move_assign(pqxx::test::context &)
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


void test_encrypt_password(pqxx::test::context &)
{
  pqxx::connection c;
  auto pw{c.encrypt_password("user", "password")};
  PQXX_CHECK(not std::empty(pw));
  PQXX_CHECK_EQUAL(
    std::strlen(pw.c_str()), std::size(pw),
    "Encrypted password contains a null byte.");
}


void test_connection_string(pqxx::test::context &)
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
  auto const connstr{c.connection_string()};

  // for (auto const &[key, value] : params)
  for (auto it{params.cbegin()}; it != params.cend(); ++it)
  {
    // Normally we'd use structured binding for this, but Facebook Infer
    // reports an insane false positive error when we do.
    auto const &key{std::get<0>(*it)};
    auto const value{std::get<1>(*it)};
    PQXX_CHECK(
      pqxx::str_contains(connstr, key),
      std::format(
        "Could not find param name '{}' in connection string: {}",
        std::string{key}, connstr));
    PQXX_CHECK(
      pqxx::str_contains(connstr, value),
      std::format(
        "Could not find value for '{}' in connection string: ",
        std::string{value}, connstr));
  }
}


void test_connection_params(pqxx::test::context &)
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


void test_raw_connection(pqxx::test::context &)
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


void test_closed_connection(pqxx::test::context &)
{
  pqxx::connection cx;
  cx.close();
  PQXX_CHECK(not cx.dbname());
  PQXX_CHECK(not cx.username());
  PQXX_CHECK(not cx.hostname());
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  PQXX_CHECK(not cx.port());
#include <pqxx/internal/ignore-deprecated-post.hxx>
  PQXX_CHECK_EQUAL(cx.port_number(), (std::optional<int>{}));
}


void test_skip_init_ssl(pqxx::test::context &)
{
  pqxx::skip_init_ssl<pqxx::skip_init::openssl, pqxx::skip_init::crypto>();
  pqxx::skip_init_ssl<pqxx::skip_init::nothing>();
}


void test_connection_client_encoding(pqxx::test::context &tctx)
{
  pqxx::connection cx;
  std::map<char const *, pqxx::encoding_group> const unsafe_encodings = {
    {"BIG5", pqxx::encoding_group::two_tier},
    {"GBK", pqxx::encoding_group::gb18030},
    {"GB18030", pqxx::encoding_group::gb18030},
    {"SJIS", pqxx::encoding_group::sjis},
    {"SHIFT_JIS_2004", pqxx::encoding_group::sjis},
    // Not actually ASCII-safe, but just close enough for our purposes.
    {"UHC", pqxx::encoding_group::ascii_safe},
  };
  for (auto const &[name, enc] : unsafe_encodings)
  {
    cx.set_client_encoding(name);
    PQXX_CHECK_EQUAL(cx.get_encoding_group(), enc);
  }

  std::vector<char const *> const safe_encodings{
    "EUC_CN",
    "EUC_JIS_2004",
    "EUC_JP",
    "EUC_KR",
    "EUC_TW",
    "ISO_8859_5",
    "ISO_8859_6",
    "ISO_8859_7",
    "ISO_8859_8",
    "KOI8R",
    "KOI8U",
    "LATIN1",
    "LATIN2",
    "LATIN3",
    "LATIN4",
    "LATIN5",
    "LATIN6",
    "LATIN7",
    "LATIN8",
    "LATIN9",
    "LATIN10",
    // For some reason setting this fails.
    // "MULE_INTERNAL",
    "SQL_ASCII",
    "UTF8",
    "WIN866",
    "WIN874",
    "WIN1250",
    "WIN1251",
    "WIN1252",
    "WIN1253",
    "WIN1254",
    "WIN1255",
    "WIN1256",
    "WIN1257",
    "WIN1258",
  };

  for (char const *const e : safe_encodings)
  {
    cx.set_client_encoding(e);
    PQXX_CHECK_EQUAL(
      cx.get_encoding_group(), pqxx::encoding_group::ascii_safe,
      std::format("Unexpected encoding group for '{}'.", e));
  }

  // Covering lots of initial letters because it helps fill out test
  // coverage on an internal switch on that initial character.
  static std::vector<char const *> const bogus_encodings{
    "ABSENT",
    "BOGUS",
    "ELUSIVE",
    "GONE",
    "ILLUSORY",
    "JOCULAR",
    "KIBOSHED",
    "LOST",
    // Actually, MULE really is a mystery because the connection does not
    // seem to accept it.
    "MYSTERY",
    "SHREDDED",
    "UNAVAILABLE",
    "WANTING",
  };

  for (char const *const e : bogus_encodings)
    PQXX_CHECK_THROWS(cx.set_client_encoding(e), pqxx::failure);

  for (int i{0}; i < 10; ++i)
  {
    std::string fake_encoding{tctx.random_char()};
    PQXX_CHECK_THROWS(cx.set_client_encoding(fake_encoding), pqxx::failure);
  }

  for (char e{'A'}; e <= 'Z'; ++e)
    PQXX_CHECK_THROWS(cx.set_client_encoding(std::string{e}), pqxx::failure);

  // We no longer support this encoding.  It turned out to be broken in
  // postgres itself.
  PQXX_CHECK_THROWS(cx.set_client_encoding("JOHAB"), pqxx::argument_error);
}


/// Simple check: does `cx` work?
void check_connection_works(pqxx::connection &cx, pqxx::test::context &tctx)
{
  pqxx::work tx{cx};
  int const value{tctx.make_num()};
  PQXX_CHECK_EQUAL(tx.query_value<int>("SELECT $1", {value}), value);
}


void test_connection_takes_string_and_params(pqxx::test::context &tctx)
{
  int const timeout{tctx.make_num(10) + 5};
  std::string const appname{tctx.make_name()};
  pqxx::connection cx{
    std::format("connect_timeout={}", timeout),
    std::map<std::string, std::string>{{"application_name", appname}}};

  check_connection_works(cx, tctx);

  // The connection combines settings from both the connection string and the
  // parameters map.
  auto const connstr{cx.connection_string()};
  PQXX_CHECK(pqxx::str_contains(connstr, "application_name"));
  PQXX_CHECK(pqxx::str_contains(connstr, appname));
  PQXX_CHECK(
    pqxx::str_contains(connstr, std::format("connect_timeout={}", timeout)));
}


void test_connection_params_override_string(pqxx::test::context &tctx)
{
  auto const first{tctx.make_name("1")}, second{tctx.make_name("2")};
  pqxx::connection cx{
    std::format("application_name={}", first),
    std::map<char const *, pqxx::zview>{{"application_name", second}}};

  check_connection_works(cx, tctx);

  auto const connstr{cx.connection_string()};
  PQXX_CHECK(not pqxx::str_contains(connstr, first));
  PQXX_CHECK(pqxx::str_contains(connstr, second));
}


void test_connection_takes_string_and_empty_params(pqxx::test::context &tctx)
{
  auto const appname{tctx.make_name()};
  pqxx::connection cx{std::format("application_name={}", appname)};

  check_connection_works(cx, tctx);

  PQXX_CHECK(pqxx::str_contains(cx.connection_string(), appname));
}


void test_connection_takes_empty_string_and_params(pqxx::test::context &tctx)
{
  auto const appname{tctx.make_name()};
  pqxx::connection cx{
    "", std::map<char const *, std::string const &>{
          {"application_name", appname}}};

  check_connection_works(cx, tctx);

  PQXX_CHECK(pqxx::str_contains(cx.connection_string(), appname));
}


void test_connection_takes_empty_string_and_empty_params(
  pqxx::test::context &tctx)
{
  pqxx::connection cx{"", std::map<std::string, std::string>{}};
  check_connection_works(cx, tctx);
}


void test_connection_rejects_bad_string(pqxx::test::context &tctx)
{
  PQXX_CHECK_THROWS(
    (pqxx::connection{tctx.make_name(), std::map<std::string, std::string>{}}),
    pqxx::broken_connection);
}


void test_connection_duplicate_params_overwrite(pqxx::test::context &tctx)
{
  auto const name1{tctx.make_name()}, name2{tctx.make_name()};
  // Use a vector here, not a map, so that we're really passing multiple
  // parameters with identical keys to the connection.
  std::vector<std::pair<char const *, pqxx::zview>> const args{
    {"application_name", name1},
    {"connect_timeout", "1"},
    {"application_name", name2},
  };

  pqxx::connection const cx{"", args};
  auto const connstr{cx.connection_string()};
  PQXX_CHECK(not pqxx::str_contains(connstr, name1));
  PQXX_CHECK(pqxx::str_contains(connstr, name2));
}


void test_quote_columns_quotes_and_escapes(pqxx::test::context &)
{
  pqxx::connection const cx;

  PQXX_CHECK_EQUAL(cx.quote_columns(std::array<std::string_view, 0u>{}), "");
  PQXX_CHECK_EQUAL(
    cx.quote_columns(std::vector<std::string>{"col"}), "\"col\"");
  std::string_view const doub[]{"aa", "bb"};
  PQXX_CHECK_EQUAL(cx.quote_columns(doub), "\"aa\",\"bb\"");

  PQXX_CHECK_EQUAL(
    cx.quote_columns(std::vector<std::string_view>{"a\"b"}), "\"a\"\"b\"");
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
PQXX_REGISTER_TEST(test_connection_client_encoding);
PQXX_REGISTER_TEST(test_quote_columns_quotes_and_escapes);
PQXX_REGISTER_TEST(test_connection_takes_string_and_params);
PQXX_REGISTER_TEST(test_connection_params_override_string);
PQXX_REGISTER_TEST(test_connection_takes_string_and_empty_params);
PQXX_REGISTER_TEST(test_connection_takes_empty_string_and_params);
PQXX_REGISTER_TEST(test_connection_takes_empty_string_and_empty_params);
PQXX_REGISTER_TEST(test_connection_rejects_bad_string);
PQXX_REGISTER_TEST(test_connection_duplicate_params_overwrite);
} // namespace
