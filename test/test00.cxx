#include <locale>

#include "test_helpers.hxx"

using namespace pqxx;


// Initial test program for libpqxx.  Test functionality that doesn't require a
// running database.

namespace
{
void check(std::string ref, std::string val, std::string vdesc)
{
  PQXX_CHECK_EQUAL(val, ref, "String mismatch for " + vdesc);
}

template<typename T> inline void strconv(
	std::string type,
	const T &Obj,
	std::string expected)
{
  const std::string Objstr{to_string(Obj)};

  check(expected, Objstr, type);
  T NewObj;
  from_string(Objstr, NewObj);
  check(expected, to_string(NewObj), "recycled " + type);
}

// There's no from_string<const char *>()...
inline void strconv(std::string type, const char Obj[], std::string expected)
{
  const std::string Objstr(to_string(Obj));
  check(expected, Objstr, type);
}

const double not_a_number = std::numeric_limits<double>::quiet_NaN();

struct intderef
{
  intderef(){}	// Silences bogus warning in some gcc versions
  template<typename ITER>
    int operator()(ITER i) const noexcept { return int(*i); }
};


void test_000()
{
  PQXX_CHECK_EQUAL(
	oid_none,
	0u,
	"InvalidIod is not zero as it used to be.  This may conceivably "
	"cause problems in libpqxx.");

  PQXX_CHECK(
	cursor_base::prior() < 0 and cursor_base::backward_all() < 0,
	"cursor_base::difference_type appears to be unsigned.");

  const char weird[] = "foo\t\n\0bar";
  const std::string weirdstr(weird, sizeof(weird)-1);

  // Test string conversions
  strconv("const char[]", "", "");
  strconv("const char[]", "foo", "foo");
  strconv("int", 0, "0");
  strconv("int", 100, "100");
  strconv("int", -1, "-1");

#if defined(_MSC_VER)
  const long long_min = LONG_MIN, long_max = LONG_MAX;
#else
  const long long_min = std::numeric_limits<long>::min(),
	long_max = std::numeric_limits<long>::max();
#endif

  std::stringstream lminstr, lmaxstr, llminstr, llmaxstr, ullmaxstr;
  lminstr.imbue(std::locale("C"));
  lmaxstr.imbue(std::locale("C"));
  llminstr.imbue(std::locale("C"));
  llmaxstr.imbue(std::locale("C"));
  ullmaxstr.imbue(std::locale("C"));

  lminstr << long_min;
  lmaxstr << long_max;

  const auto ullong_max = std::numeric_limits<unsigned long long>::max();
  const auto
	llong_max = std::numeric_limits<long long>::max(),
	llong_min = std::numeric_limits<long long>::min();

  llminstr << llong_min;
  llmaxstr << llong_max;
  ullmaxstr << ullong_max;

  strconv("long", 0, "0");
  strconv("long", long_min, lminstr.str());
  strconv("long", long_max, lmaxstr.str());
  strconv("double", not_a_number, "nan");
  strconv("string", std::string{}, "");
  strconv("string", weirdstr, weirdstr);
  strconv("long long", 0LL, "0");
  strconv("long long", llong_min, llminstr.str());
  strconv("long long", llong_max, llmaxstr.str());
  strconv("unsigned long long", 0ULL, "0");
  strconv("unsigned long long", ullong_max, ullmaxstr.str());

  const char zerobuf[] = "0";
  std::string zero;
  from_string(zerobuf, zero, sizeof(zerobuf)-1);
  PQXX_CHECK_EQUAL(
	zero,
	zerobuf,
	"Converting \"0\" with explicit length failed.");

  const char nulbuf[] = "\0string\0with\0nuls\0";
  const std::string nully(nulbuf, sizeof(nulbuf)-1);
  std::string nully_parsed;
  from_string(nulbuf, nully_parsed, sizeof(nulbuf)-1);
  PQXX_CHECK_EQUAL(nully_parsed.size(), nully.size(), "Nul truncates string.");
  PQXX_CHECK_EQUAL(nully_parsed, nully, "String conversion breaks on nuls.");
  from_string(nully.c_str(), nully_parsed, nully.size());
  PQXX_CHECK_EQUAL(nully_parsed, nully, "Nul conversion breaks on strings.");

  std::stringstream ss;
  strconv("empty stringstream", ss, "");
  ss << -3.1415;
  strconv("stringstream", ss, ss.str());

  // TODO: Test binarystring reversibility

  const std::string pw = encrypt_password("foo", "bar");
  PQXX_CHECK(not pw.empty(), "Encrypting a password returned no data.");
  PQXX_CHECK_NOT_EQUAL(
	pw,
	encrypt_password("splat", "blub"),
	"Password encryption is broken.");
  PQXX_CHECK(
	pw.find("bar") == std::string::npos,
	"Encrypted password contains original.");

  // Test error handling for failed connections
  {
    nullconnection nc;
    PQXX_CHECK_THROWS(
    	work w(nc),
	broken_connection,
	"nullconnection fails to fail.");
  }
  {
    nullconnection nc("");
    PQXX_CHECK_THROWS(
	work w(nc),
	broken_connection,
	"nullconnection(const char[]) is broken.");
  }
  {
    std::string n;
    nullconnection nc(n);
    PQXX_CHECK_THROWS(
	work w(nc),
	broken_connection,
	"nullconnection(const string &) is broken.");
  }

  // Now that we know nullconnections throw errors as expected, test
  // pqxx_exception.
  try
  {
    nullconnection nc;
    work w(nc);
  }
  catch (const pqxx_exception &e)
  {
    PQXX_CHECK(
	dynamic_cast<const broken_connection *>(&e.base()),
	"Downcast pqxx_exception is not a broken_connection");
    pqxx::test::expected_exception(e.base().what());
    PQXX_CHECK_EQUAL(
	dynamic_cast<const broken_connection &>(e.base()).what(),
        e.base().what(),
	"Inconsistent what() message in exception.");
  }
}


PQXX_REGISTER_TEST(test_000);
} // namespace
