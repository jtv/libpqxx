#include <pqxx/transaction>

#include "helpers.hxx"

// Test program for libpqxx array parsing.

using namespace std::literals;

namespace pqxx
{
inline std::string to_string(pqxx::array_parser::juncture const &j)
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
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
#include "pqxx/internal/ignore-deprecated-post.hxx"
}
} // namespace pqxx


namespace
{
void test_empty_arrays()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;

  // Parsing a null pointer just immediately returns "done".
  output = pqxx::array_parser(std::string_view()).get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");

  // Parsing an empty array string immediately returns "done".
  output = pqxx::array_parser("").get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");

  // Parsing an empty array returns "row_start", "row_end", "done".
  pqxx::array_parser empty_parser("{}");
  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);
  PQXX_CHECK_EQUAL(output.second, "");

  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = empty_parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_array_null_value()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser containing_null("{NULL}");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);
  PQXX_CHECK_EQUAL(output.second, "");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::null_value);
  PQXX_CHECK_EQUAL(output.second, "");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = containing_null.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_array_double_quoted_string()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{\"item\"}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "item");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_array_double_quoted_escaping()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser(R"--({"don''t\\ care"})--");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "don''t\\ care");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


// A pair of double quotes in a double-quoted string is an escaped quote.
void test_array_double_double_quoted_string()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser{R"--({"3"" steel"})--"};

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);

  PQXX_CHECK_EQUAL(output.second, "3\" steel");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_array_unquoted_string()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{item}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "item");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_array_multiple_values()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{1,2}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "1");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "2");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_nested_array()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{{item}}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "item");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_nested_array_with_multiple_entries()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::pair<pqxx::array_parser::juncture, std::string> output;
  pqxx::array_parser parser("{{1,2},{3,4}}");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "1");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "2");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_start);

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "3");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::string_value);
  PQXX_CHECK_EQUAL(output.second, "4");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(output.second, "");

  output = parser.get_next();
  PQXX_CHECK_EQUAL(output.first, pqxx::array_parser::juncture::done);
  PQXX_CHECK_EQUAL(output.second, "");
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_generate_empty_array()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};
  PQXX_CHECK_EQUAL(pqxx::to_string(std::vector<int>{}, c), "{}");
  PQXX_CHECK_EQUAL(pqxx::to_string(std::vector<std::string>{}, c), "{}");
}


void test_generate_null_value()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<char const *>{nullptr}, c), "{NULL}");
}


void test_generate_single_item()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};
  PQXX_CHECK_EQUAL(pqxx::to_string(std::vector<int>{42}, c), "{42}");

  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<char const *>{"foo"}, c), "{\"foo\"}");
}


void test_generate_multiple_items()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<int>{5, 4, 3, 2}, c), "{5,4,3,2}");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{"foo", "bar"}, c),
    "{\"foo\",\"bar\"}");
}


void test_generate_nested_array()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::vector<int>>{{1, 2}, {3, 4}}, c),
    "{{1,2},{3,4}}");
}


void test_generate_escaped_strings()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{"a\\b"}, c), "{\"a\\\\b\"}",
    "Backslashes are not escaped properly.");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{"x\"y\""}, c), "{\"x\\\"y\\\"\"}",
    "Double quotes are not escaped properly.");
}


void test_array_generate_empty_strings()
{
  pqxx::conversion_context const c{pqxx::encoding_group::ascii_safe};

  // Reproduce #816: Under-budgeted conversion of empty strings in arrays.
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>{{""}}, c), "{\"\"}");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(std::vector<std::string>({"", "", "", ""}), c),
    "{\"\",\"\",\"\",\"\"}");
  PQXX_CHECK_EQUAL(
    pqxx::to_string(
      std::vector<std::string>(
        {"", "", "", "", "", "", "", "", "", "", "", ""}),
      c),
    "{\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\",\"\"}");
}

void test_sparse_arrays()
{
  // Reproduce #922 : NULL not paying for its separator in an array, causing
  // problems in sparse arrays.

  // If NULL didn't pay for its separator, the size allocated for an array-like
  // object filled with null-like values would be too small.

  auto arrayOfNulls = std::vector<std::optional<int>>(4, std::nullopt);
  std::string const arrayOfNullsStr = "{NULL,NULL,NULL,NULL}";

  PQXX_CHECK_GREATER_EQUAL(
    pqxx::size_buffer(arrayOfNulls), arrayOfNullsStr.size(),
    "Buffer size allocated for an array of optional<int> filled with nulls "
    "was too small.");

  PQXX_CHECK_EQUAL(
    pqxx::to_string(arrayOfNulls), arrayOfNullsStr,
    "Array of optional<int> filled with std::nullopt came out wrong. ");


  // If the array-like object is instead sparsely-filled, the values contained
  // in non-null elements may leave behind excess unused size which hides the
  // problem better - it only becomes an error once the array contains enough
  // null-like values to outweigh this excess unused size.

  std::array<std::optional<int>, 14> sparseArray;
  sparseArray[sparseArray.size() - 1] = 42;

  std::string const sparseArrayStr =
    "{NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,"
    "42}";

  PQXX_CHECK_GREATER_EQUAL(
    pqxx::size_buffer(sparseArray), sparseArrayStr.size(),
    "Buffer size allocated for a sparsely-filled array of optional<int> was "
    "too small.");

  PQXX_CHECK_EQUAL(pqxx::to_string(sparseArray), sparseArrayStr);
}

void test_array_roundtrip()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  pqxx::connection cx;
  pqxx::work tx{cx};

  std::vector<int> const in{0, 1, 2, 3, 5};
  auto const text{
    tx.query_value<std::string>("SELECT $1::integer[]", pqxx::params{in})};
  pqxx::array_parser parser{text};
  auto item{parser.get_next()};
  PQXX_CHECK_EQUAL(item.first, pqxx::array_parser::juncture::row_start);

  std::vector<int> out;
  for (item = parser.get_next();
       item.first == pqxx::array_parser::juncture::string_value;
       item = parser.get_next())
  {
    out.push_back(pqxx::from_string<int>(item.second));
  }

  PQXX_CHECK_EQUAL(item.first, pqxx::array_parser::juncture::row_end);
  PQXX_CHECK_EQUAL(std::size(out), std::size(in));

  for (std::size_t i{0}; i < std::size(in); ++i)
    PQXX_CHECK_EQUAL(out[i], in[i]);

  item = parser.get_next();
  PQXX_CHECK_EQUAL(item.first, pqxx::array_parser::juncture::done);
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


void test_array_strings()
{
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  std::vector<std::string_view> const inputs{
    "",    "null", "NULL", "\\N", "'",    "''", "\\", "\n\t",
    "\\n", "\"",   "\"\"", "a b", "a<>b", "{",  "}",  "{}",
  };
  pqxx::connection cx;
  pqxx::work tx{cx};

  for (auto const &input : inputs)
  {
    auto const f{tx.exec("SELECT ARRAY[$1]", pqxx::params{input}).one_field()};
    pqxx::array_parser parser{f.as<std::string_view>()};
    [[maybe_unused]] auto [start_juncture, start_value]{parser.get_next()};
    PQXX_CHECK_EQUAL(start_juncture, pqxx::array_parser::juncture::row_start);
    auto [value_juncture, value]{parser.get_next()};
    PQXX_CHECK_EQUAL(
      value_juncture, pqxx::array_parser::juncture::string_value);
    PQXX_CHECK_EQUAL(value, input);
  }
#include "pqxx/internal/ignore-deprecated-post.hxx"
}


namespace
{
pqxx::conversion_context make_context(
  pqxx::encoding_group enc = pqxx::encoding_group::unknown,
  pqxx::sl loc = pqxx::sl::current())
{
  return pqxx::conversion_context{enc, loc};
}
} // namespace

void test_array_parses_real_arrays()
{
  auto const mono{pqxx::encoding_group::ascii_safe};

  pqxx::connection cx;
  pqxx::work tx{cx};

  PQXX_CHECK_THROWS(
    (std::ignore = pqxx::from_string<pqxx::array<int>>("{}")),
    pqxx::usage_error,
    "Array parser accepted text in an unknown encoding group.");

  auto const empty_s{tx.query_value<std::string>("SELECT ARRAY[]::integer[]")};
  pqxx::array<int> const empty_a{
    pqxx::from_string<pqxx::array<int>>(empty_s, make_context(mono))};
  PQXX_CHECK_EQUAL(empty_a.dimensions(), 1u);
  PQXX_CHECK_EQUAL(empty_a.sizes(), (std::array<std::size_t, 1u>{0u}));

  auto const onedim_s{tx.query_value<std::string>("SELECT ARRAY[0, 1, 2]")};
  pqxx::array<int> const onedim_a{
    pqxx::from_string<pqxx::array<int>>(onedim_s, make_context(mono))};
  PQXX_CHECK_EQUAL(onedim_a.dimensions(), 1u);
  PQXX_CHECK_EQUAL(onedim_a.sizes(), (std::array<std::size_t, 1u>{3u}));
  PQXX_CHECK_EQUAL(onedim_a.at(0), 0);
  PQXX_CHECK_EQUAL(onedim_a[0], 0);
  PQXX_CHECK_EQUAL(onedim_a.at(2), 2);
  PQXX_CHECK_EQUAL(onedim_a[2], 2);

  auto const null_s{
    tx.query_value<std::string>("SELECT ARRAY[NULL]::integer[]")};
  PQXX_CHECK_THROWS((pqxx::array<int>{null_s, cx}), pqxx::unexpected_null);

  auto const twodim_s{tx.query_value<std::string>("SELECT ARRAY[[1], [2]]")};
  pqxx::array<int, 2> const twodim_a{
    pqxx::from_string<pqxx::array<int, 2>>(twodim_s, make_context(mono))};
  PQXX_CHECK_EQUAL(twodim_a.dimensions(), 2u);
  PQXX_CHECK_EQUAL(twodim_a.sizes(), (std::array<std::size_t, 2>{2u, 1u}));

  auto const string_s{tx.query_value<std::string>("SELECT ARRAY['Hello']")};
  pqxx::array<std::string> const string_a{string_s, cx};
  PQXX_CHECK_EQUAL(string_a[0], "Hello");

  auto const fake_null_s{tx.query_value<std::string>("SELECT ARRAY['NULL']")};
  pqxx::array<std::string> const fake_null_a{string_s, cx};
  PQXX_CHECK_EQUAL(fake_null_a[0], "Hello");

  auto const nulls_s{
    tx.query_value<std::string>("SELECT ARRAY[NULL, 'NULL']")};
  pqxx::array<std::optional<std::string>> const nulls_a{nulls_s, cx};
  PQXX_CHECK(not nulls_a[0].has_value());
  PQXX_CHECK(nulls_a[1].has_value());
  PQXX_CHECK_EQUAL(nulls_a[1].value_or("(missing)"), "NULL");
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
  pqxx::connection const cx;
  pqxx::array<std::string> const a{
    R"x({"","n","nnn","\"'","""","\\","\"","a""","""z"})x", cx};
  PQXX_CHECK_EQUAL(a.at(0), "");
  PQXX_CHECK_EQUAL(a.at(1), "n");
  PQXX_CHECK_EQUAL(a.at(2), "nnn");
  PQXX_CHECK_EQUAL(a.at(3), R"x("')x");
  PQXX_CHECK_EQUAL(a.at(4), R"x(")x");
  PQXX_CHECK_EQUAL(a.at(5), "\\");
  PQXX_CHECK_EQUAL(a.at(6), "\"");
  PQXX_CHECK_EQUAL(a.at(7), "a\"");
  PQXX_CHECK_EQUAL(a.at(8), "\"z");

  // A byte value that looks like an ASCII backslash but inside a multibyte
  // character does not count as a backslash.
  pqxx::array<std::string> const b{
    "{\"\203\\\",\"\\\203\\\"}", pqxx::encoding_group::sjis};
  PQXX_CHECK_EQUAL(
    b.at(0),
    "\203\\"
    "");
  // If encoding support didn't work properly, puting a backslash in front
  // would probably only get applied to the first byte in the character, and
  // turn that embedded byte bcak into a backslash.
  PQXX_CHECK_EQUAL(
    b.at(1),
    "\203\\"
    "");
}


void test_array_parses_multidim_arrays()
{
  pqxx::connection const cx;
  pqxx::array<int, 2u> const a{"{{0,1},{2,3}}", cx};
  PQXX_CHECK_EQUAL(a.at(0u, 0u), 0);
  PQXX_CHECK_EQUAL(a.at(1u, 0u), 2);
  PQXX_CHECK_EQUAL(a.at(1u, 1u), 3);
}


void test_array_at_checks_bounds()
{
  pqxx::connection const cx;
  pqxx::array<int> const simple{"{0, 1, 2}", cx};
  PQXX_CHECK_EQUAL(simple.at(0), 0);
  PQXX_CHECK_EQUAL(simple.at(2), 2);
  PQXX_CHECK_THROWS(simple.at(3), pqxx::range_error);
  PQXX_CHECK_THROWS(simple.at(-1), pqxx::range_error);

  pqxx::array<int, 2> const multi{"{{0,1},{2,3},{4,5}}", cx};
  PQXX_CHECK_EQUAL(multi.at(0, 0), 0);
  PQXX_CHECK_EQUAL(multi.at(1, 1), 3);
  PQXX_CHECK_EQUAL(multi.at(2, 1), 5);
  PQXX_CHECK_THROWS(multi.at(3, 0), pqxx::range_error);
  PQXX_CHECK_THROWS(multi.at(0, 2), pqxx::range_error);
  PQXX_CHECK_THROWS(multi.at(0, -1), pqxx::range_error);
  PQXX_CHECK_THROWS(multi.at(-1, 0), pqxx::range_error);
}


void test_array_iterates_in_row_major_order()
{
  pqxx::connection cx;
  pqxx::work tx{cx};
  auto const array_s{tx.query_value<std::string>(
    "SELECT ARRAY[[1, 2, 3], [4, 5, 6], [7, 8, 9]]")};
  pqxx::array<int, 2> const array{array_s, cx};
  auto it{array.cbegin()};
  PQXX_CHECK_EQUAL(*it, 1);
  ++it;
  ++it;
  PQXX_CHECK_EQUAL(*it, 3);
  ++it;
  PQXX_CHECK_EQUAL(*it, 4);
  it += 6;
  PQXX_CHECK(it == array.cend());
  PQXX_CHECK_EQUAL(*(array.cend() - 1), 9);
  PQXX_CHECK_EQUAL(*array.crbegin(), 9);
  PQXX_CHECK_EQUAL(*(array.crend() - 1), 1);
  PQXX_CHECK_EQUAL(std::size(array), 9u);
  PQXX_CHECK_EQUAL(std::ssize(array), 9);
  PQXX_CHECK_EQUAL(array.front(), 1);
  PQXX_CHECK_EQUAL(array.back(), 9);
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


// Shorthand for shorthand for std::source_location::current().
constexpr pqxx::sl here(pqxx::sl loc = pqxx::sl::current())
{
  return loc;
}


void test_scan_double_quoted_string()
{
  using enc = pqxx::encoding_group;

  // TODO: Use static_assert() once Visual Studio handles it.
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("")", 0u, here()),
    2u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"(""z)", 0u, here()),
    2u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"(x="")", 2u, here()),
    4u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"(x=""z)", 2u, here()),
    4u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("x")", 0u, here()),
    3u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("x"z)", 0u, here()),
    3u);
  PQXX_CHECK_THROWS(
    (pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("foo)", 0u, here())),
    pqxx::argument_error,
    "Double-quoted string scan did not detect missing closing quote.");
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      "\"x\\\"y\"", 0u, here()),
    6u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      "\"x\\\"y\"z\"", 0u, here()),
    6u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("x\\y")", 0u, here()),
    6u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("x""y")", 0u, here()),
    6u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("x""y"z)", 0u, here()),
    6u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      "\"\\\\\\\"\"\"\"", 0u, here()),
    8u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      "a\"\\\\\\\"\"\"\"", 1u, here()),
    9u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"("""")", 0u, here()),
    4u);
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::ascii_safe>(
      R"(""""z)", 0u, here()),
    4u);

  // Now let's try a byte that _looks_ like an ASCII backslash escaping the
  // closing quote (which would be an obvious vector for an injection attack)
  // but is actually just one byte in a multibyte character.
  // (I believe these two SJIS bytes form the Katakana letter "so".)
  PQXX_CHECK_EQUAL(
    pqxx::internal::scan_double_quoted_string<enc::sjis>(
      "\"\203\\\"suffix", 0u, here()),
    4u, "Fell for embedded ASCII-like byte in multibyte char.");
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
PQXX_REGISTER_TEST(test_scan_double_quoted_string);
PQXX_REGISTER_TEST(test_generate_empty_array);
PQXX_REGISTER_TEST(test_generate_null_value);
PQXX_REGISTER_TEST(test_generate_single_item);
PQXX_REGISTER_TEST(test_generate_multiple_items);
PQXX_REGISTER_TEST(test_generate_nested_array);
PQXX_REGISTER_TEST(test_generate_escaped_strings);
PQXX_REGISTER_TEST(test_sparse_arrays);
} // namespace
