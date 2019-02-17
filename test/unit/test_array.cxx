#include "../test_helpers.hxx"

using namespace pqxx;


// Test program for libpqxx array parsing.

namespace
{
void test_empty_arrays()
{
  std::pair<array_parser::juncture, std::string> output;

  // Parsing a null pointer just immediately returns "done".
  output = array_parser(nullptr).get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "get_next on null array did not return done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  // Parsing an empty array string immediately returns "done".
  output = array_parser("").get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "get_next on an empty array string did not return done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  // Parsing an empty array returns "row_start", "row_end", "done".
  array_parser empty_parser("{}");
  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(
          int(output.first),
          int(array_parser::juncture::row_start),
          "Empty array did not start with row_start.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(
          int(output.first),
          int(array_parser::juncture::row_end),
          "Empty array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(
          int(output.first),
          int(array_parser::juncture::done),
          "Empty array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_null_value()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser containing_null("{NULL}");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
      int(output.first),
      int(array_parser::juncture::row_start),
      "Array containing null did not start with row_start.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
      int(output.first),
      int(array_parser::juncture::null_value),
      "Array containing null did not return null_value.");
  PQXX_CHECK_EQUAL(output.second, "", "Null value was not empty.");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
      int(output.first),
      int(array_parser::juncture::row_end),
      "Array containing null did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
          int(output.first),
          int(array_parser::juncture::done),
          "Array containing null did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

}


void test_single_quoted_string()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{'item'}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_single_quoted_escaping()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{'don''t\\\\ care'}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(
        output.second, "don't\\ care", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_double_quoted_string()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{\"item\"}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_double_quoted_escaping()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{\"don''t\\\\ care\"}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(
        output.second, "don''t\\ care", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_unquoted_string()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{item}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_multiple_values()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{1,2}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "1", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "2", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_nested_array()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{{item}}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Nested array did not start 2nd dimension with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Nested array did not end 2nd dimension with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_nested_array_with_multiple_entries()
{
  std::pair<array_parser::juncture, std::string> output;
  array_parser parser("{{1,2},{3,4}}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Nested array did not start 2nd dimension with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "1", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "2", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Nested array did not end 2nd dimension with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_start),
        "Nested array did not descend to 2nd dimension with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "3", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::string_value),
        "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "4", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Nested array did not leave 2nd dimension with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::row_end),
        "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
        int(output.first),
        int(array_parser::juncture::done),
        "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_array_parse()
{
  test_empty_arrays();
  test_null_value();
  test_single_quoted_string();
  test_single_quoted_escaping();
  test_double_quoted_string();
  test_double_quoted_escaping();
  test_unquoted_string();
  test_multiple_values();
  test_nested_array();
  test_nested_array_with_multiple_entries();
}


PQXX_REGISTER_TEST(test_array_parse);
} // namespace
