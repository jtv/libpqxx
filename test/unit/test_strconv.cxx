#include "../test_helpers.hxx"

namespace
{
void test_strconv_bool()
{
  PQXX_CHECK_EQUAL(pqxx::to_string(false), "false", "Wrong to_string(false).");
  PQXX_CHECK_EQUAL(pqxx::to_string(true), "true", "Wrong to_string(true).");

  bool result;
  pqxx::from_string("false", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('false').");
  pqxx::from_string("FALSE", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('FALSE').");
  pqxx::from_string("f", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('f').");
  pqxx::from_string("F", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('F').");
  pqxx::from_string("0", result);
  PQXX_CHECK_EQUAL(result, false, "Wrong from_string('0').");
  pqxx::from_string("true", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('true').");
  pqxx::from_string("TRUE", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('TRUE').");
  pqxx::from_string("t", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('t').");
  pqxx::from_string("T", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('T').");
  pqxx::from_string("1", result);
  PQXX_CHECK_EQUAL(result, true, "Wrong from_string('1').");
}


PQXX_REGISTER_TEST(test_strconv_bool);
}
