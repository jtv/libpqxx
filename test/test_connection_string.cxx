#include <pqxx/pqxx>

#include "helpers.hxx"


namespace
{
constexpr std::string_view const marker{"application_name="};


/// Connect to the database, passing `app_name` for its `application_name`.
/** Does not do any quoting or escaping.  The caller will have to do that.
 */
pqxx::connection connect(std::string const &app_name)
{
  return pqxx::connection(std::string{marker} + app_name);
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
  std::string const &all{cx.connection_string()};

  auto const intro{all.find(marker)};
  auto const start{intro + std::size(marker)};
  if (start == std::string::npos or all.at(start) == ' ')
    return "";
  std::size_t here{start}, end{};
  if (all.at(here) == '\'')
  {
    // Quoted string.
    ++here;
    for (bool esc{false};
         (here < std::size(all)) and ((all.at(here) != '\'') or esc); ++here)
      esc = (all.at(here) == '\\' and not esc);
    assert(here < std::size(all));
    assert(all.at(here) == '\'');
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


void check_connect_string(std::string const &in, std::string const &expected)
{
  auto cx{connect(in)};
  PQXX_CHECK_EQUAL(app_name(cx), expected);

  // Check that connection_string() produced a valid, more or less equivalent
  // connection string.
  pqxx::connection{cx.connection_string()};
}


void test_connection_string_escapes(pqxx::test::context &)
{
  check_connect_string("pqxxtest", "pqxxtest");
  check_connect_string("'hello'", "hello");
  check_connect_string("'a b c'", "'a b c'");
  check_connect_string("'x \\\\y'", "'x \\\\y'");

  // TODO: Use raw strings once Visual Studio copes with backslashes there.

  // NOLINTNEXTLINE(modernize-raw-string-literal)
  check_connect_string("\\\\r\\\\n", "\\\\r\\\\n");

  // This does seem to get quoted, even though as I read the spec, that's not
  // actually required because there's no space in it.
  check_connect_string("don\\'t", "'don\\'t'");
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


PQXX_REGISTER_TEST(test_connection_string_escapes);
PQXX_REGISTER_TEST(test_connection_string_parser_accepts_empty_string);
PQXX_REGISTER_TEST(test_connection_string_parser_accepts_connection_string);
PQXX_REGISTER_TEST(test_connection_string_parser_deduplicates);
PQXX_REGISTER_TEST(test_connection_string_parser_unquotes);
PQXX_REGISTER_TEST(test_connection_string_parser_unescapes);
} // namespace
