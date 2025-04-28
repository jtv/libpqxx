#include <format>

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
    // Quoted string.  Be prepared for escaping.
    ++here;
    for (bool esc{false};
         (here < std::size(all)) and ((all.at(here) != '\'') or esc); ++here)
      esc = (all.at(here) == '\\' and not esc);
    assert(here < std::size(all) - 1);
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


void test_connection_string_escapes()
{
  // XXX: Deal with encodings?
  // XXX: Deal with URI encoding?
  PQXX_CHECK_EQUAL(
    app_name(connect("pqxxtest")), "pqxxtest",
    "Simple connection param came back wrong.");
  PQXX_CHECK_EQUAL(
    app_name(connect("'hello'")), "hello",
    "Quoted connection param came back wrong.");
  PQXX_CHECK_EQUAL(
    app_name(connect("'a b c'")), "'a b c'", "Spaces in param went bad.");
  PQXX_CHECK_EQUAL(
    app_name(connect("'x \\\\y'")), "'x \\\\y'", "Backslash escaping failed.");
  PQXX_CHECK_EQUAL(app_name(connect("\\\\r\\\\n")), "\\\\r\\\\n", "Unnecessary quotes for backslash?");
  // This does seem to get quoted, even though as I read the spec, that's not
  // actually required.
  PQXX_CHECK_EQUAL(
    app_name(connect("don\\'t")), "'don\\'t'",
    "Unexpected backslash escaping.");
}


PQXX_REGISTER_TEST(test_connection_string_escapes);
} // namespace
