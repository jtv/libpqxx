#include <pqxx/transaction>

#include "../test_helpers.hxx"

// Test program for libpqxx array parsing.

using namespace std::literals;

namespace pqxx
{
template<>
struct nullness<array_parser::juncture> : no_null<array_parser::juncture>
{};


inline std::string to_string(pqxx::array_parser::juncture const &j)
{
  using junc = pqxx::array_parser::juncture;
  switch (j)
  {
  case junc::row_start: return "row_start";
  case junc::row_end: return "row_end";
  case junc::null_value: return "null_value";
  case junc::string_value: return "string_value";
  case junc::done: return "done";
  default: return "UNKNOWN JUNCTURE: " + to_string(static_cast<int>(j));
  }
}
} // namespace pqxx


namespace
{
void test_empty_arrays()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;

  // Parsing a null pointer just immediately returns "done".
  output = pqxx::array_parser(std::string_view()).get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "get_next on null array did not return done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  // Parsing an empty array string immediately returns "done".
  output = pqxx::array_parser("").get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "get_next on an empty array string did not return done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  // Parsing an empty array returns "row_start", "row_end", "done".
  pqxx::array_parser empty_parser("{}");
  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Empty array did not start with row_start.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Empty array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Empty array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_array_null_value()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser containing_null("{NULL}");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array containing null did not start with row_start.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::null_value,
    "Array containing null did not return null_value.");
  PQXX_CHECK_EQUAL(output.second, "", "Null value was not empty.");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array containing null did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array containing null did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_array_double_quoted_string()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{\"item\"}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_array_double_quoted_escaping()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser(R"--({"don''t\\ care"})--");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "don''t\\ care", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


// A pair of double quotes in a double-quoted string is an escaped quote.
void test_array_double_double_quoted_string()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser{R"--({"3"" steel"})--"};

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");

  PQXX_CHECK_EQUAL(output.second, "3\" steel", "Unexpected string value.");
}


void test_array_unquoted_string()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{item}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_array_multiple_values()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{1,2}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "1", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "2", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_nested_array()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{{item}}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Nested array did not start 2nd dimension with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "item", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Nested array did not end 2nd dimension with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_nested_array_with_multiple_entries()
{
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{{1,2},{3,4}}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Nested array did not start 2nd dimension with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "1", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "2", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Nested array did not end 2nd dimension with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_start,
    "Nested array did not descend to 2nd dimension with row_start.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "3", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::string_value,
    "Array did not return string_value.");
  PQXX_CHECK_EQUAL(output.second, "4", "Unexpected string value.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Nested array did not leave 2nd dimension with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::row_end,
    "Array did not end with row_end.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(
    output.first, pqxx::array_parser::juncture::done,
    "Array did not conclude with done.");
  PQXX_CHECK_EQUAL(output.second, "", "Unexpected nonempty output.");
}


void test_generate_empty_array()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<int>{}), "{}",
    "Basic array output is not as expected.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{}), "{}",
    "String array comes out different.");
}


void test_generate_null_value()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<char const *>{nullptr}), "{NULL}",
    "Null array value did not come out as expected.");
}


void test_generate_single_item()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<int>{42}), "{42}",
    "Numeric conversion came out wrong.");

  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<char const *>{"foo"}), "{\"foo\"}",
    "String array conversion came out wrong.");
}


void test_generate_multiple_items()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<int>{5, 4, 3, 2}), "{5,4,3,2}",
    "Array with multiple values is not correct.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{"foo", "bar"}),
    "{\"foo\",\"bar\"}", "Array with multiple strings came out wrong.");
}


void test_generate_nested_array()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::vector<int>>{{1, 2}, {3, 4}}),
    "{{1,2},{3,4}}", "Nested arrays don't work right.");
}


void test_generate_escaped_strings()
{
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{"a\\b"}), "{\"a\\\\b\"}",
    "Backslashes are not escaped properly.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{"x\"y\""}), "{\"x\\\"y\\\"\"}",
    "Double quotes are not escaped properly.");
}


void test_array_generate_empty_strings()
{
  // Reproduce #816: Under-budgeted conversion of empty strings in arrays.
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>({""})), "{\"\"}",
    "Array of one empty string came out wrong.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>({"", "", "", ""})),
    "{\"\",\"\",\"\",\"\"}", "Array of 4 empty strings came out wrong.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>(
      {"", "", "", "", "", "", "", "", "", "", "", ""})),
    "{\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"}",
    "Array of 12 empty strings came out wrong.");
}


void test_array_generate()
{
  test_generate_empty_array();
  test_generate_null_value();
  test_generate_single_item();
  test_generate_multiple_items();
  test_generate_nested_array();
  test_generate_escaped_strings();
}


void test_array_roundtrip()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::vector<int> const in{0, 1, 2, 3, 5};
  auto const text{
    tx.query_value<std::string>("SELECT $1::integer[]", pqxx::params{in})};
  pqxx::array_parser parser{text};
  auto item{parser.get_next()};
  PQXX_CHECK_EQUAL(
    item.first, pqxx::array_parser::juncture::row_start,
    "Array did not start with row_start.");

  std::vector<int> out;
  for (item = parser.get_next();
       item.first == pqxx::array_parser::juncture::string_value;
       item = parser.get_next())
  {
    out.push_back(pqxx::from_string<int>(item.second));
  }

  PQXX_CHECK_EQUAL(
    item.first, pqxx::array_parser::juncture::row_end,
    "Array values did not end in row_end.");
  PQXX_CHECK_EQUAL(
    std::size(out), std::size(in), "Array came back with different length.");

  for (std::size_t i{0}; i < std::size(in); ++i)
    PQXX_CHECK_EQUAL(out[i], in[i], "Array element has changed.");

  item = parser.get_next();
  PQXX_CHECK_EQUAL(
    item.first, pqxx::array_parser::juncture::done,
    "Array did not end in done.");
}


void test_array_strings()
{
  std::vector<std::string_view> inputs{
    "",    "null", "NULL", "\\N", "'",    "''", "\\", "\n\t",
    "\\n", "\"",   "\"\"", "a b", "a<>b", "{",  "}",  "{}",
  };
  pqxx::connection cx;
  pqxx::work tx{cx};

  for (auto const &input : inputs)
  {
    auto const f{tx.exec("SELECT ARRAY[$1]", pqxx::params{input}).one_field()};
    pqxx::array_parser parser{f.as<std::string_view>()};
    auto [start_juncture, start_value]{parser.get_next()};
    pqxx::ignore_unused(start_value);
    PQXX_CHECK_EQUAL(
      start_juncture, pqxx::array_parser::juncture::row_start, "Bad start.");
    auto [value_juncture, value]{parser.get_next()};
    PQXX_CHECK_EQUAL(
      value_juncture, pqxx::array_parser::juncture::string_value,
      "Bad value juncture.");
    PQXX_CHECK_EQUAL(value, input, "Bad array value roundtrip.");
  }
}


void test_array_parses_real_arrays()
{
  pqxx::connection cx;
  pqxx::work tx{cx};

  auto const empty_s{tx.query_value<std::string>("SELECT ARRAY[]::integer[]")};
  pqxx::array<int> empty_a{empty_s, cx};
  PQXX_CHECK_EQUAL(
    empty_a.dimensions(), 1u, "Unexpected dimension count for empty array.");
  PQXX_CHECK_EQUAL(
    empty_a.sizes(), (std::array<std::size_t, 1u>{0u}),
    "Unexpected sizes for empty array.");

  auto const onedim_s{tx.query_value<std::string>("SELECT ARRAY[0, 1, 2]")};
  pqxx::array<int> onedim_a{onedim_s, cx};
  PQXX_CHECK_EQUAL(
    onedim_a.dimensions(), 1u,
    "Unexpected dimension count for one-dimensional array.");
  PQXX_CHECK_EQUAL(
    onedim_a.sizes(), (std::array<std::size_t, 1u>{3u}),
    "Unexpected sizes for one-dimensional array.");
  PQXX_CHECK_EQUAL(onedim_a[0], 0, "Bad data in one-dimensional array.");
  PQXX_CHECK_EQUAL(
    onedim_a[2], 2, "Array started off OK but later data was bad.");

  auto const null_s{
    tx.query_value<std::string>("SELECT ARRAY[NULL]::integer[]")};
  PQXX_CHECK_THROWS(
    (pqxx::array<int>{null_s, cx}), pqxx::unexpected_null,
    "Not getting unexpected_null from array parser.");

  auto const twodim_s{tx.query_value<std::string>("SELECT ARRAY[[1], [2]]")};
  pqxx::array<int, 2> twodim_a{twodim_s, cx};
  PQXX_CHECK_EQUAL(
    twodim_a.dimensions(), 2u,
    "Wrong number of dimensions on multi-dimensional array.");
  PQXX_CHECK_EQUAL(
    twodim_a.sizes(), (std::array<std::size_t, 2>{2u, 1u}),
    "Wrong sizes on multidim array.");

  auto const string_s{tx.query_value<std::string>("SELECT ARRAY['Hello']")};
  pqxx::array<std::string> string_a{string_s, cx};
  PQXX_CHECK_EQUAL(string_a[0], "Hello", "String field came out wrong.");

  auto const fake_null_s{tx.query_value<std::string>("SELECT ARRAY['NULL']")};
  pqxx::array<std::string> fake_null_a{string_s, cx};
  PQXX_CHECK_EQUAL(
    fake_null_a[0], "Hello", "String field 'NULL' came out wrong.");

  auto const nulls_s{
    tx.query_value<std::string>("SELECT ARRAY[NULL, 'NULL']")};
  pqxx::array<std::optional<std::string>> nulls_a{nulls_s, cx};
  PQXX_CHECK(not nulls_a[0].has_value(), "Null string cvame out with value.");
  PQXX_CHECK(nulls_a[1].has_value(), "String 'NULL' came out as null.");
  PQXX_CHECK_EQUAL(
    nulls_a[1].value(), "NULL", "String 'NULL' came out wrong.");
}


void test_array_rejects_malformed_simple_int_arrays()
{
  pqxx::connection cx;
  std::string_view const bad_arrays[]{
    ""sv,     "null"sv, ","sv,      "1"sv,    "{"sv,         "}"sv,   "}{"sv,
    "{}{"sv,  "{{}"sv,  "{}}"sv,    "{{}}"sv, "{1"sv,        "{1,"sv, "{,}"sv,
    "{1,}"sv, "{,1}"sv, "{1,{}}"sv, "{x}"sv,  "{1,{2,3}}"sv,
  };
  for (auto bad : bad_arrays)
    PQXX_CHECK_THROWS(
      (pqxx::array<int>{bad, cx}), pqxx::conversion_error,
      "No conversion_error for '" + std::string{bad} + "'.");
}


void test_array_rejects_malformed_simple_string_arrays()
{
  pqxx::connection cx;
  std::string_view const bad_arrays[]{
    ""sv,    "null"sv, "1"sv,    ","sv,    "{"sv,      "}"sv,
    "}{"sv,  "{}{"sv,  "{{}"sv,  "{}}"sv,  "{{}}"sv,   "{1"sv,
    "{1,"sv, "{,}"sv,  "{1,}"sv, "{,1}"sv, "{1,{}}"sv,
  };
  for (auto bad : bad_arrays)
    PQXX_CHECK_THROWS(
      (pqxx::array<std::string>{bad, cx}), pqxx::conversion_error,
      "No conversion_error for '" + std::string{bad} + "'.");
}


void test_array_rejects_malformed_twodimensional_arrays()
{
  pqxx::connection cx;
  std::string_view const bad_arrays[]{
    ""sv,
    "{}"sv,
    "{null}"sv,
    "{{1},{2,3}}"sv,
  };
  for (auto bad : bad_arrays)
    PQXX_CHECK_THROWS(
      (pqxx::array<std::string, 2>{bad, cx}), pqxx::conversion_error,
      "No conversion_error for '" + std::string{bad} + "'.");
}


void test_array_parses_quoted_strings()
{
  pqxx::connection cx;
  pqxx::array<std::string> const a{R"x({"\"'"})x", cx};
  PQXX_CHECK_EQUAL(a[0], R"x("')x", "String in array did not unescape right.");
}


void test_array_parses_multidim_arrays()
{
  pqxx::connection cx;
  pqxx::array<int, 2u> const a{"{{0,1},{2,3}}", cx};
  PQXX_CHECK_EQUAL(a.at(0u, 0u), 0, "Indexing is wrong.");
  PQXX_CHECK_EQUAL(a.at(1u, 0u), 2, "Indexing seems to confuse dimensions.");
  PQXX_CHECK_EQUAL(a.at(1u, 1u), 3, "Indexing at higher indexes goes wrong.");
}


void test_array_at_checks_bounds()
{
  pqxx::connection cx;
  pqxx::array<int> const simple{"{0, 1, 2}", cx};
  PQXX_CHECK_EQUAL(simple.at(0), 0, "Array indexing does not work.");
  PQXX_CHECK_EQUAL(simple.at(2), 2, "Nonzero array indexing goes wrong.");
  PQXX_CHECK_THROWS(
    simple.at(3), pqxx::range_error, "No bounds checking on array::at().");
  PQXX_CHECK_THROWS(
    simple.at(-1), pqxx::range_error,
    "Negative index does not throw range_error.");

  pqxx::array<int, 2> const multi{"{{0,1},{2,3},{4,5}}", cx};
  PQXX_CHECK_EQUAL(
    multi.at(0, 0), 0, "Multidim array indexing does not work.");
  PQXX_CHECK_EQUAL(multi.at(1, 1), 3, "Nonzero multidim indexing goes wrong.");
  PQXX_CHECK_EQUAL(multi.at(2, 1), 5, "Multidim top element went wrong.");
  PQXX_CHECK_THROWS(
    multi.at(3, 0), pqxx::range_error,
    "Out-of-bounds on outer dimension was not detected.");
  PQXX_CHECK_THROWS(
    multi.at(0, 2), pqxx::range_error,
    "Out-of-bounds on inner dimension was not detected.");
  PQXX_CHECK_THROWS(
    multi.at(0, -1), pqxx::range_error,
    "Negative inner index was not detected.");
  PQXX_CHECK_THROWS(
    multi.at(-1, 0), pqxx::range_error,
    "Negative outer index was not detected.");
}


void test_array_iterates_in_row_major_order()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const array_s{tx.query_value<std::string>(
    "SELECT ARRAY[[1, 2, 3], [4, 5, 6], [7, 8, 9]]")};
  pqxx::array<int, 2> array{array_s, cx};
  auto it{array.cbegin()};
  PQXX_CHECK_EQUAL(*it, 1, "Iteration started off wrong.");
  ++it;
  ++it;
  PQXX_CHECK_EQUAL(*it, 3, "Iteration seems to have taken the wrong order.");
  ++it;
  PQXX_CHECK_EQUAL(*it, 4, "Iteration did not jump to the next dimension.");
  it += 6;
  PQXX_CHECK(it == array.cend(), "Array cend() not where I expected.");
  PQXX_CHECK_EQUAL(*(array.cend() - 1), 9, "Iteration did not end well.");
  PQXX_CHECK_EQUAL(*array.crbegin(), 9, "Bad crbegin().");
  PQXX_CHECK_EQUAL(*(array.crend() - 1), 1, "Bad crend().");
  PQXX_CHECK_EQUAL(std::size(array), 9u, "Bad array size.");
  // C++20: Use std::ssize() instead.
  PQXX_CHECK_EQUAL(array.ssize(), 9, "Bad array ssize().");
  PQXX_CHECK_EQUAL(array.front(), 1, "Bad front().");
  PQXX_CHECK_EQUAL(array.back(), 9, "Bad back().");
}


void test_as_sql_array()
{
  pqxx::connection cx;
  pqxx::row r;
  {
    pqxx::work tx{cx};
    r = tx.exec("SELECT ARRAY [5, 4, 3, 2]").one_row();
    // Connection closes, but we should still be able to parse the array.
  }
  auto const array{r[0].as_sql_array<int>()};
  PQXX_CHECK_EQUAL(array[1], 4, "Got wrong value out of array.");
}


PQXX_REGISTER_TEST(test_empty_arrays);
PQXX_REGISTER_TEST(test_array_null_value);
PQXX_REGISTER_TEST(test_array_double_quoted_string);
PQXX_REGISTER_TEST(test_array_double_quoted_escaping);
PQXX_REGISTER_TEST(test_array_double_double_quoted_string);
PQXX_REGISTER_TEST(test_array_unquoted_string);
PQXX_REGISTER_TEST(test_array_multiple_values);
PQXX_REGISTER_TEST(test_nested_array);
PQXX_REGISTER_TEST(test_nested_array_with_multiple_entries);
PQXX_REGISTER_TEST(test_array_generate);
PQXX_REGISTER_TEST(test_array_roundtrip);
PQXX_REGISTER_TEST(test_array_strings);
PQXX_REGISTER_TEST(test_array_parses_real_arrays);
PQXX_REGISTER_TEST(test_array_rejects_malformed_simple_int_arrays);
PQXX_REGISTER_TEST(test_array_rejects_malformed_simple_string_arrays);
PQXX_REGISTER_TEST(test_array_rejects_malformed_twodimensional_arrays);
PQXX_REGISTER_TEST(test_array_parses_quoted_strings);
PQXX_REGISTER_TEST(test_array_parses_multidim_arrays);
PQXX_REGISTER_TEST(test_array_at_checks_bounds);
PQXX_REGISTER_TEST(test_array_iterates_in_row_major_order);
PQXX_REGISTER_TEST(test_array_generate_empty_strings);
PQXX_REGISTER_TEST(test_as_sql_array);
} // namespace
