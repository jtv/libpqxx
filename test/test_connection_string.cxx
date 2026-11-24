#include <pqxx/pqxx>

#include "helpers.hxx"


namespace
{
/// Connect to the database, passing `app_name` for its `application_name`.
/** Does not do any quoting or escaping.  The caller will have to do that.
 */
pqxx::connection
connect(std::string const &app_name, pqxx::sl loc = pqxx::sl::current())
{
  return pqxx::connection{std::format("application_name={}", app_name), loc};
}


/// Extract the application name from a connection's connection string.
/** Does not do any un-escaping, does not remove quotes.  This is deliberate.
 *
 * Also does not handle much in the way of weird inputs.  It just looks for
 * the first instance of "application_name=" and goes from there.  This is
 * because I'm lazy and I see no substantial security risks in this.
 */
std::string app_name(pqxx::connection const &cx)
{
  constexpr std::string_view const marker{"application_name="};
  std::string const &all{cx.connection_string()};

  auto const intro{all.find(marker)};
  auto start{intro + std::size(marker)};
  if (start == std::string::npos)
    return "";

  // Space around the equals sign is optional (although we assume for finding
  // the entry we need that there's none before the equals sign).
  for (; std::isspace(all.at(start)); ++start)
    if (start == std::size(all))
      return "";

  std::size_t here{start}, end{};
  if (all.at(here) == '\'')
  {
    // Quoted string.
    ++here;
    for (bool esc{false};
         (here < std::size(all)) and ((all.at(here) != '\'') or esc); ++here)
      esc = (all.at(here) == '\\' and not esc);

    // clang-tidy rule bug:
    // NOLINTBEGIN(cert-dcl03-c)
    assert(here < std::size(all));
    assert(all.at(here) == '\'');
    // NOLINTEND(cert-dcl03-c)

    // Skip closing quote.
    end = here + 1;
  }
  else
  {
    // Simple string.  Just stop at the next space, or end of string.
    // This find() can return npos, but that's fine here.
    end = all.find(' ', start);
  }
  return all.substr(start, end - start);
}


/// Check that the connection's application name is as expected.
void check_connect_string(
  std::string const &in, std::string const &expected,
  pqxx::sl loc = pqxx::sl::current())
{
  auto const cx{connect(in, loc)};
  PQXX_CHECK_EQUAL(
    app_name(cx), expected, "Connection string value did not match.", loc);

  // Check that connection_string() produced a valid, more or less equivalent
  // connection string.
  pqxx::connection{cx.connection_string()};
}


void test_connection_string_escapes(pqxx::test::context &)
{
  // If there are no special characters, quotes are optional.
  check_connect_string("hello", "hello");
  check_connect_string("'hello'", "hello");

  // Embedded spaces need quoting (but no escaping).
  check_connect_string("x.y", "x.y");
  check_connect_string("'x y'", "'x y'");
  check_connect_string("'x\ty'", "'x\ty'");
  check_connect_string("'x\ny'", "'x\ny'");
  check_connect_string("'x\ry'", "'x\ry'");
  check_connect_string("'x\fy'", "'x\fy'");
  check_connect_string("'x\vy'", "'x\vy'");

  // Tricky: leading & trailing space.
  check_connect_string(" ' foo ' ", "' foo '");

  // This gets quoted because of the space, not because of the backslashes.
  check_connect_string("'x \\\\y'", "'x \\\\y'");

  // TODO: Use raw strings once Visual Studio copes with backslashes there.

  // NOLINTNEXTLINE(modernize-raw-string-literal)
  check_connect_string("\\\\r\\\\n", "\\\\r\\\\n");

  // Embedded quotes need escaping (but not necessarily quoting).
  check_connect_string("don\\'t", "don\\'t");
}


/// Convenience alias for a long, long name.
using parser = pqxx::internal::connection_string_parser;


void test_connection_string_parser_accepts_empty_string(pqxx::test::context &)
{
  parser const p{""};
  auto const [keys, values]{p.parse()};
  PQXX_CHECK(keys.empty());
  PQXX_CHECK(values.empty());
}


void test_connection_string_parser_accepts_connection_string(
  pqxx::test::context &tctx)
{
  int const timeout{tctx.make_num(10) + 5};
  parser const p{std::format("connect_timeout={}", timeout).c_str()};
  auto const [keys, values]{p.parse()};
  PQXX_CHECK_EQUAL(std::size(keys), std::size(values));
  PQXX_CHECK_EQUAL(std::size(keys), 1u);
  PQXX_CHECK_EQUAL(std::string{keys.at(0)}, "connect_timeout");
  PQXX_CHECK_EQUAL(pqxx::from_string<int>(values.at(0)), timeout);
}


void test_connection_string_parser_deduplicates(pqxx::test::context &tctx)
{
  auto const name1{tctx.make_name()}, name2{tctx.make_name()};
  parser const p{
    std::format("application_name={} application_name={}", name1, name2)
      .c_str()};
  auto const [keys, values]{p.parse()};
  PQXX_CHECK_EQUAL(std::size(keys), std::size(values));
  PQXX_CHECK_EQUAL(std::size(keys), 1u);
  PQXX_CHECK_EQUAL((std::string{keys.at(0)}), "application_name");
  PQXX_CHECK_EQUAL((std::string{values.at(0)}), name2);
}


void test_connection_string_parser_unquotes(pqxx::test::context &)
{
  parser const p1{"application_name='q u o t e d'"};
  auto const [k1, v1]{p1.parse()};
  PQXX_CHECK_EQUAL(std::size(k1), 1u);
  PQXX_CHECK_EQUAL(std::string{v1.at(0)}, "q u o t e d");
}


void test_connection_string_parser_unescapes(pqxx::test::context &)
{
  parser const p1{"application_name=can\\'t"};
  auto const [k1, v1]{p1.parse()};
  PQXX_CHECK_EQUAL(std::size(k1), 1u);
  PQXX_CHECK_EQUAL(std::string{v1.at(0)}, "can't");
}


void test_connection_string_can_be_zview(pqxx::test::context &)
{
  pqxx::connection const cx_without_params{pqxx::zview{}};
  PQXX_CHECK(cx_without_params.is_open());

  pqxx::connection const cx_with_params{
    pqxx::zview{}, std::array<std::pair<pqxx::zview, std::string>, 1u>{
                     {{"connect_timeout", "5"}}}};
  PQXX_CHECK(cx_with_params.is_open());
}


void test_connection_string_can_be_c_string(pqxx::test::context &)
{
  pqxx::connection const cx_without_params{""};
  PQXX_CHECK(cx_without_params.is_open());

  pqxx::connection const cx_with_params{
    "", std::array<std::pair<pqxx::zview, std::string>, 1u>{
          {{"connect_timeout", "5"}}}};
  PQXX_CHECK(cx_with_params.is_open());
}


void test_connection_string_can_be_string(pqxx::test::context &)
{
  pqxx::connection const cx_without_params{std::string{}};
  PQXX_CHECK(cx_without_params.is_open());

  pqxx::connection const cx_with_params{
    std::string{}, std::array<std::pair<pqxx::zview, std::string>, 1u>{
                     {{"connect_timeout", "5"}}}};
  PQXX_CHECK(cx_with_params.is_open());
}


PQXX_REGISTER_TEST(test_connection_string_escapes);
PQXX_REGISTER_TEST(test_connection_string_parser_accepts_empty_string);
PQXX_REGISTER_TEST(test_connection_string_parser_accepts_connection_string);
PQXX_REGISTER_TEST(test_connection_string_parser_deduplicates);
PQXX_REGISTER_TEST(test_connection_string_parser_unquotes);
PQXX_REGISTER_TEST(test_connection_string_parser_unescapes);
PQXX_REGISTER_TEST(test_connection_string_can_be_zview);
PQXX_REGISTER_TEST(test_connection_string_can_be_c_string);
PQXX_REGISTER_TEST(test_connection_string_can_be_string);
} // namespace
