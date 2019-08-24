#include "../test_helpers.hxx"

namespace
{
void test_type_name()
{
  // It's hard to test in more detail, because spellings may differ.
  // For instance, one compiler might call "const unsigned int" what another
  // might call "unsigned const".
  PQXX_CHECK_EQUAL(
	pqxx::type_name<int>,
	"int",
	"type_name<int> came out weird.");
  PQXX_CHECK_EQUAL(
	pqxx::type_name<std::shared_ptr<int>>,
	"std::shared_ptr<int>",
	"type_name<std::shared_ptr<int>> came out weird.");
}


PQXX_REGISTER_TEST(test_type_name);
} // namespace
