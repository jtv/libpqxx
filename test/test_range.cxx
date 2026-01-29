#include <pqxx/range>
#include <pqxx/strconv>

#include "helpers.hxx"


namespace
{
void test_range_construct(pqxx::test::context &)
{
  using optint = std::optional<int>;
  using oibound = pqxx::inclusive_bound<std::optional<int>>;
  using oxbound = pqxx::inclusive_bound<std::optional<int>>;
  PQXX_CHECK_THROWS(
    (pqxx::range<optint>{oibound{optint{}}, oibound{optint{}}}),
    pqxx::argument_error);
  PQXX_CHECK_THROWS(
    (pqxx::range<optint>{oxbound{optint{}}, oxbound{optint{}}}),
    pqxx::argument_error);

  using ibound = pqxx::inclusive_bound<int>;
  PQXX_CHECK_THROWS(
    (pqxx::range<int>{ibound{1}, ibound{0}}), pqxx::range_error);

  PQXX_CHECK_THROWS(
    (pqxx::range<float>{
      pqxx::inclusive_bound<float>{-1000.0},
      pqxx::inclusive_bound<float>{-std::numeric_limits<float>::infinity()}}),
    pqxx::range_error);
}


void test_range_equality(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using ibound = pqxx::inclusive_bound<int>;
  using xbound = pqxx::exclusive_bound<int>;
  using ubound = pqxx::no_bound;

  PQXX_CHECK_EQUAL(range{}, range{});
  PQXX_CHECK_EQUAL(
    (range{xbound{0}, xbound{0}}), (range{xbound{5}, xbound{5}}));

  PQXX_CHECK_EQUAL((range{ubound{}, ubound{}}), (range{ubound{}, ubound{}}));
  PQXX_CHECK_EQUAL(
    (range{ibound{5}, ibound{8}}), (range{ibound{5}, ibound{8}}));
  PQXX_CHECK_EQUAL(
    (range{xbound{5}, xbound{8}}), (range{xbound{5}, xbound{8}}));
  PQXX_CHECK_EQUAL(
    (range{xbound{5}, ibound{8}}), (range{xbound{5}, ibound{8}}));
  PQXX_CHECK_EQUAL(
    (range{ibound{5}, xbound{8}}), (range{ibound{5}, xbound{8}}));
  PQXX_CHECK_EQUAL((range{ubound{}, ibound{8}}), (range{ubound{}, ibound{8}}));
  PQXX_CHECK_EQUAL((range{ibound{8}, ubound{}}), (range{ibound{8}, ubound{}}));

  PQXX_CHECK_NOT_EQUAL(
    (range{ibound{5}, ibound{8}}), (range{xbound{5}, ibound{8}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{ibound{5}, ibound{8}}), (range{ubound{}, ibound{8}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{xbound{5}, ibound{8}}), (range{ubound{}, ibound{8}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{ibound{5}, ibound{8}}), (range{ibound{5}, xbound{8}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{ibound{5}, ibound{8}}), (range{ibound{5}, ubound{}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{ibound{5}, xbound{8}}), (range{ibound{5}, ubound{}}));

  PQXX_CHECK_NOT_EQUAL(
    (range{ibound{5}, ibound{8}}), (range{ibound{4}, ibound{8}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{xbound{5}, ibound{8}}), (range{xbound{4}, ibound{8}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{xbound{5}, ibound{8}}), (range{xbound{5}, ibound{7}}));
  PQXX_CHECK_NOT_EQUAL(
    (range{xbound{5}, xbound{8}}), (range{xbound{5}, xbound{7}}));
}


void test_range_empty(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using ibound = pqxx::inclusive_bound<int>;
  using xbound = pqxx::exclusive_bound<int>;
  using ubound = pqxx::no_bound;
  PQXX_CHECK((range{}.empty()));
  PQXX_CHECK((range{ibound{10}, xbound{10}}).empty());
  PQXX_CHECK((range{xbound{10}, ibound{10}}).empty());
  PQXX_CHECK((range{xbound{10}, xbound{10}}).empty());

  PQXX_CHECK(not(range{ibound{10}, ibound{10}}).empty());
  PQXX_CHECK(not(range{xbound{10}, ibound{11}}.empty()));
  PQXX_CHECK(not(range{ubound{}, ubound{}}.empty()));
  PQXX_CHECK(not(range{ubound{}, xbound{0}}.empty()));
  PQXX_CHECK(not(range{xbound{0}, ubound{}}.empty()));
}


void test_range_contains(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using ibound = pqxx::inclusive_bound<int>;
  using xbound = pqxx::exclusive_bound<int>;
  using ubound = pqxx::no_bound;

  PQXX_CHECK(not(range{}.contains(-1)));
  PQXX_CHECK(not(range{}.contains(0)));
  PQXX_CHECK(not(range{}.contains(1)));

  PQXX_CHECK(not(range{ibound{5}, ibound{8}}.contains(4)));
  PQXX_CHECK((range{ibound{5}, ibound{8}}.contains(5)));
  PQXX_CHECK((range{ibound{5}, ibound{8}}.contains(6)));
  PQXX_CHECK((range{ibound{5}, ibound{8}}.contains(8)));
  PQXX_CHECK(not(range{ibound{5}, ibound{8}}.contains(9)));

  PQXX_CHECK(not(range{ibound{5}, xbound{8}}.contains(4)));
  PQXX_CHECK((range{ibound{5}, xbound{8}}.contains(5)));
  PQXX_CHECK((range{ibound{5}, xbound{8}}.contains(6)));
  PQXX_CHECK(not(range{ibound{5}, xbound{8}}.contains(8)));
  PQXX_CHECK(not(range{ibound{5}, xbound{8}}.contains(9)));

  PQXX_CHECK(not(range{xbound{5}, ibound{8}}.contains(4)));
  PQXX_CHECK(not(range{xbound{5}, ibound{8}}.contains(5)));
  PQXX_CHECK((range{xbound{5}, ibound{8}}.contains(6)));
  PQXX_CHECK((range{xbound{5}, ibound{8}}.contains(8)));
  PQXX_CHECK(not(range{xbound{5}, ibound{8}}.contains(9)));

  PQXX_CHECK(not(range{xbound{5}, xbound{8}}.contains(4)));
  PQXX_CHECK(not(range{xbound{5}, xbound{8}}.contains(5)));
  PQXX_CHECK((range{xbound{5}, xbound{8}}.contains(6)));
  PQXX_CHECK(not(range{xbound{5}, xbound{8}}.contains(8)));
  PQXX_CHECK(not(range{xbound{5}, xbound{8}}.contains(9)));

  PQXX_CHECK((range{ubound{}, ibound{8}}.contains(7)));
  PQXX_CHECK((range{ubound{}, ibound{8}}.contains(8)));
  PQXX_CHECK(not(range{ubound{}, ibound{8}}.contains(9)));

  PQXX_CHECK((range{ubound{}, xbound{8}}.contains(7)));
  PQXX_CHECK(not(range{ubound{}, xbound{8}}.contains(8)));
  PQXX_CHECK(not(range{ubound{}, xbound{8}}.contains(9)));

  PQXX_CHECK(not(range{ibound{5}, ubound{}}.contains(4)));
  PQXX_CHECK((range{ibound{5}, ubound{}}.contains(5)));
  PQXX_CHECK((range{ibound{5}, ubound{}}.contains(6)));

  PQXX_CHECK(not(range{xbound{5}, ubound{}}.contains(4)));
  PQXX_CHECK(not(range{xbound{5}, ubound{}}.contains(5)));
  PQXX_CHECK((range{xbound{5}, ubound{}}.contains(6)));

  PQXX_CHECK((range{ubound{}, ubound{}}.contains(-1)));
  PQXX_CHECK((range{ubound{}, ubound{}}.contains(0)));
  PQXX_CHECK((range{ubound{}, ubound{}}.contains(1)));
}


void test_float_range_contains(pqxx::test::context &)
{
  using range = pqxx::range<double>;
  using ibound = pqxx::inclusive_bound<double>;
  using xbound = pqxx::exclusive_bound<double>;
  using ubound = pqxx::no_bound;
  using limits = std::numeric_limits<double>;
  constexpr auto inf{limits::infinity()};

  PQXX_CHECK(not(range{ibound{4.0}, ibound{8.0}}.contains(3.9)));
  PQXX_CHECK((range{ibound{4.0}, ibound{8.0}}.contains(4.0)));
  PQXX_CHECK((range{ibound{4.0}, ibound{8.0}}.contains(5.0)));

  PQXX_CHECK((range{ibound{0}, ibound{inf}}).contains(9999.0));
  PQXX_CHECK(not(range{ibound{0}, ibound{inf}}.contains(-0.1)));
  PQXX_CHECK((range{ibound{0}, xbound{inf}}.contains(9999.0)));
  PQXX_CHECK((range{ibound{0}, ibound{inf}}).contains(inf));
  PQXX_CHECK(not(range{ibound{0}, xbound{inf}}.contains(inf)));
  PQXX_CHECK((range{ibound{0}, ubound{}}).contains(inf));

  PQXX_CHECK((range{ibound{-inf}, ibound{0}}).contains(-9999.0));
  PQXX_CHECK(not(range{ibound{-inf}, ibound{0}}.contains(0.1)));
  PQXX_CHECK((range{xbound{-inf}, ibound{0}}).contains(-9999.0));
  PQXX_CHECK((range{ibound{-inf}, ibound{0}}).contains(-inf));
  PQXX_CHECK(not(range{xbound{-inf}, ibound{0}}).contains(-inf));
  PQXX_CHECK((range{ubound{}, ibound{0}}).contains(-inf));
}


void test_range_subset(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using traits = pqxx::string_traits<range>;

  constexpr std::string_view subsets[][2]{
    {"empty", "empty"},  {"(,)", "empty"},    {"(0,1)", "empty"},
    {"(,)", "[-10,10]"}, {"(,)", "(-10,10)"}, {"(,)", "(,)"},
    {"(,10)", "(,10)"},  {"(,10)", "(,9)"},   {"(,10]", "(,10)"},
    {"(,10]", "(,10]"},  {"(1,)", "(10,)"},   {"(1,)", "(9,)"},
    {"[1,)", "(10,)"},   {"[1,)", "[10,)"},   {"[0,5]", "[1,4]"},
    {"(0,5)", "[1,4]"},
  };
  for (auto const &[super, sub] : subsets)
    PQXX_CHECK(
      traits::from_string(super).contains(traits::from_string(sub)),
      std::format("Range '{}' did not contain ''.", super, sub));

  constexpr std::string_view non_subsets[][2]{
    {"empty", "[0,0]"},   {"empty", "(,)"},     {"[-10,10]", "(,)"},
    {"(-10,10)", "(,)"},  {"(,9)", "(,10)"},    {"(,10)", "(,10]"},
    {"[1,4]", "[0,4]"},   {"[1,4]", "[1,5]"},   {"(0,10)", "[0,10]"},
    {"(0,10)", "(0,10]"}, {"(0,10)", "[0,10)"},
  };
  for (auto const &[super, sub] : non_subsets)
    PQXX_CHECK(
      not traits::from_string(super).contains(traits::from_string(sub)),
      std::format("Range '{}' contained '{}'.", super, sub));
}


void test_range_to_string(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using ibound = pqxx::inclusive_bound<int>;
  using xbound = pqxx::exclusive_bound<int>;
  using ubound = pqxx::no_bound;

  PQXX_CHECK_EQUAL(pqxx::to_string(range{}), "empty");

  PQXX_CHECK_EQUAL(pqxx::to_string(range{ibound{5}, ibound{8}}), "[5,8]");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{xbound{5}, ibound{8}}), "(5,8]");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{ibound{5}, xbound{8}}), "[5,8)");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{xbound{5}, xbound{8}}), "(5,8)");

  // Unlimited boundaries can use brackets or parentheses.  Doesn't matter.
  // We cheat and use some white-box knowledge of our implementation here.
  PQXX_CHECK_EQUAL(pqxx::to_string(range{ubound{}, ubound{}}), "(,)");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{ubound{}, ibound{8}}), "(,8]");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{ubound{}, xbound{8}}), "(,8)");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{ibound{5}, ubound{}}), "[5,)");
  PQXX_CHECK_EQUAL(pqxx::to_string(range{xbound{5}, ubound{}}), "(5,)");
}


void test_parse_range(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using ubound = pqxx::no_bound;
  using traits = pqxx::string_traits<range>;

  constexpr std::string_view empties[]{"empty", "EMPTY", "eMpTy"};
  for (auto empty : empties)
    PQXX_CHECK(
      traits::from_string(empty).empty(),
      std::format("This was supposed to produce an empty range: '{}'", empty));

  constexpr std::string_view universals[]{"(,)", "[,)", "(,]", "[,]"};
  for (auto univ : universals)
    PQXX_CHECK_EQUAL(
      traits::from_string(univ), (range{ubound{}, ubound{}}),
      std::format(
        "This was supposed to produce a universal range: '{}'", univ));

  PQXX_CHECK(traits::from_string("(0,10]").lower_bound().is_exclusive());
  PQXX_CHECK(traits::from_string("[0,10]").lower_bound().is_inclusive());
  PQXX_CHECK(traits::from_string("(0,10)").upper_bound().is_exclusive());
  PQXX_CHECK(traits::from_string("[0,10]").upper_bound().is_inclusive());

  PQXX_CHECK_EQUAL(
    *traits::from_string("(\"0\",\"10\")").lower_bound().value(), 0);
  PQXX_CHECK_EQUAL(
    *traits::from_string("(\"0\",\"10\")").upper_bound().value(), 10);

  auto floats{
    pqxx::string_traits<pqxx::range<double>>::from_string("(0,1.0)")};
  PQXX_CHECK_GREATER(*floats.lower_bound().value(), -0.001);
  PQXX_CHECK_LESS(*floats.lower_bound().value(), 0.001);
  PQXX_CHECK_GREATER(*floats.upper_bound().value(), 0.999);
  PQXX_CHECK_LESS(*floats.upper_bound().value(), 1.001);
}


void test_parse_bad_range(pqxx::test::context &)
{
  using range = pqxx::range<int>;
  using conv_err = pqxx::conversion_error;
  using traits = pqxx::string_traits<range>;
  constexpr std::string_view bad_ranges[]{
    "",   "x",       "e",          "empt",       "emptyy",   "()",
    "[]", "(empty)", "(empty, 0)", "(0, empty)", ",",        "(,",
    ",)", "(1,2,3)", "(4,5x)",     "(null, 0)",  "[0, 1.0]", "[1.0, 0]",
  };

  for (auto bad : bad_ranges)
    PQXX_CHECK_THROWS(
      std::ignore = traits::from_string(bad), conv_err,
      std::format("This range wasn't supposed to parse: '{}'", bad));
}


/// Parse ranges lhs and rhs, return their intersection as a string.
template<typename TYPE>
std::string intersect(std::string_view lhs, std::string_view rhs)
{
  using traits = pqxx::string_traits<pqxx::range<TYPE>>;
  return pqxx::to_string(traits::from_string(lhs) & traits::from_string(rhs));
}


void test_range_intersection(pqxx::test::context &)
{
  // Intersections and their expected results, in text form.
  // Each row contains two ranges, and their intersection.
  constexpr std::string_view intersections[][3]{
    {"empty", "empty", "empty"},
    {"(,)", "empty", "empty"},
    {"[,]", "empty", "empty"},
    {"empty", "[0,10]", "empty"},
    {"(,)", "(,)", "(,)"},
    {"(,)", "(5,8)", "(5,8)"},
    {"(,)", "[5,8)", "[5,8)"},
    {"(,)", "(5,8]", "(5,8]"},
    {"(,)", "[5,8]", "[5,8]"},
    {"(-1000,10)", "(0,1000)", "(0,10)"},
    {"[-1000,10)", "(0,1000)", "(0,10)"},
    {"(-1000,10]", "(0,1000)", "(0,10]"},
    {"[-1000,10]", "(0,1000)", "(0,10]"},
    {"[0,100]", "[0,100]", "[0,100]"},
    {"[0,100]", "[0,100)", "[0,100)"},
    {"[0,100]", "(0,100]", "(0,100]"},
    {"[0,100]", "(0,100)", "(0,100)"},
    {"[0,10]", "[11,20]", "empty"},
    {"[0,10]", "(11,20]", "empty"},
    {"[0,10]", "[11,20)", "empty"},
    {"[0,10]", "(11,20)", "empty"},
    {"[0,10]", "[10,11]", "[10,10]"},
    {"[0,10)", "[10,11]", "empty"},
    {"[0,10]", "(10,11]", "empty"},
    {"[0,10)", "(10,11]", "empty"},
  };
  for (auto const &[left, right, expected] : intersections)
  {
    PQXX_CHECK_EQUAL(
      intersect<int>(left, right), expected,
      std::format(
        "Intersection of '{}' and '{}' produced unexpected result.", left,
        right));
    PQXX_CHECK_EQUAL(
      intersect<int>(right, left), expected,
      std::format(
        "Intersection of '{}' and '{}' was asymmetric.", left, right));
  }
}


void test_range_conversion(pqxx::test::context &)
{
  std::array<std::string_view, 8> const ranges{
    "empty", "(,)", "(,10)", "(0,)", "[0,10]", "[0,10)", "(0,10]", "(0,10)",
  };

  for (auto r : ranges)
  {
    auto const shortr{pqxx::from_string<pqxx::range<short>>(r)};
    pqxx::range<int> const intr{shortr};
    PQXX_CHECK_EQUAL(pqxx::to_string(intr), r);
  }
}


constexpr void test_range_is_constexpr(pqxx::test::context &)
{
  // Test compile-time operations.
  using range = pqxx::range<int>;
  using ibound = pqxx::inclusive_bound<int>;

  constexpr ibound one{1}, three{3};
  constexpr range oneone{one, one}, onethree{one, three};
  static_assert(oneone == range{oneone});
  static_assert(oneone != onethree);
  static_assert(onethree.contains(oneone));
}


PQXX_REGISTER_TEST(test_range_construct);
PQXX_REGISTER_TEST(test_range_equality);
PQXX_REGISTER_TEST(test_range_empty);
PQXX_REGISTER_TEST(test_range_contains);
PQXX_REGISTER_TEST(test_float_range_contains);
PQXX_REGISTER_TEST(test_range_subset);
PQXX_REGISTER_TEST(test_range_to_string);
PQXX_REGISTER_TEST(test_parse_range);
PQXX_REGISTER_TEST(test_parse_bad_range);
PQXX_REGISTER_TEST(test_range_intersection);
PQXX_REGISTER_TEST(test_range_conversion);
PQXX_REGISTER_TEST(test_range_is_constexpr);
} // namespace
