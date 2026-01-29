#include "helpers.hxx"

namespace
{
void test_type_name(pqxx::test::context &)
{
#include <pqxx/internal/ignore-deprecated-pre.hxx>
  // It's hard to test in more detail, because spellings may differ.
  // For instance, one compiler might call "const unsigned int*" what another
  // might call "unsigned const *".  And Visual Studio prefixes "class" to
  // class types.
  auto const &i{pqxx::type_name<int>};
  PQXX_CHECK_LESS(std::size(i), 5u, "type_name<int> is suspiciously long.");
  PQXX_CHECK_EQUAL(
    i.substr(0, 1), "i", "type_name<int> does not start with 'i'.");
#include <pqxx/internal/ignore-deprecated-post.hxx>
}


void test_name_type(pqxx::test::context &)
{
  // We have a few hand-defined type names.
  PQXX_CHECK_EQUAL(pqxx::name_type<std::string>(), "std::string");
  PQXX_CHECK_EQUAL(pqxx::name_type<std::string_view>(), "std::string_view");
  PQXX_CHECK_EQUAL(pqxx::name_type<int>(), "int");
}


PQXX_REGISTER_TEST(test_type_name);
PQXX_REGISTER_TEST(test_name_type);
} // namespace
