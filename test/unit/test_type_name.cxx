#include "../test_helpers.hxx"

namespace
{
void test_type_name()
{
  // It's hard to test in more detail, because spellings may differ.
  // For instance, one compiler might call "const unsigned int" what another
  // might call "unsigned const".  And Visual Studio prefixes "class" to class
  // types.
  PQXX_CHECK_EQUAL(
    pqxx::type_name<int>, "int", "type_name<int> came out weird.");
}


PQXX_REGISTER_TEST(test_type_name);
} // namespace
