#include <pqxx/pqxx>

#include "test_helpers.hxx"


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
  std::size_t here{start}, end;
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
    here = all.find(' ', start);
    end = all.find(' ', start);
  }
  return all.substr(start, end - start);
}


void check_connect_string(std::string const &in, std::string const &expected)
{
  auto cx{connect(in)};
  PQXX_CHECK_EQUAL(
    app_name(cx), expected, "App name did not come out as expected.");

  // Check that connection_string() produced a valid, more or less equivalent
  // connection string.
  pqxx::connection{cx.connection_string()};
}


void test_connection_string_escapes()
{
  // XXX: Deal with encodings?
  // XXX: Deal with URI encoding?
  check_connect_string("pqxxtest", "pqxxtest");
  check_connect_string("'hello'", "hello");
  check_connect_string("'a b c'", "'a b c'");
  check_connect_string("'x \\\\y'", "'x \\\\y'");
  check_connect_string("\\\\r\\\\n", "\\\\r\\\\n");

  // This does seem to get quoted, even though as I read the spec, that's not
  // actually required because there's no space in it.
  check_connect_string("don\\'t", "'don\\'t'");
}


PQXX_REGISTER_TEST(test_connection_string_escapes);
} // namespace
