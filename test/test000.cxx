// Test "long long" support, if available
#define PQXX_ALLOW_LONG_LONG

// We need some libpqxx-internal configuration items.  DON'T INCLUDE THIS HEADER
// IN NORMAL CLIENT PROGRAMS!
#include "pqxx/config-internal-compiler.h"

#ifdef PQXX_HAVE_LOCALE
#include <locale>
#endif

#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


// Initial test program for libpqxx.  Test functionality that doesn't require a
// running database.

namespace
{
template<typename T> void testitems(const T &I, typename T::size_type s)
{
  PQXX_CHECK_EQUAL(I.size(), s, "Wrong size in items class.");

  for ( ; s; --s)
  {
    const typename T::size_type value = typename T::size_type((I[s-1]));
    PQXX_CHECK_EQUAL(value, s, "Wrong value in items object.");
  }
}

void check(string ref, string val, string vdesc)
{
  PQXX_CHECK_EQUAL(val, ref, "String mismatch for " + vdesc);
}

template<typename T> inline void strconv(string type,
    const T &Obj,
    string expected)
{
  const string Objstr(to_string(Obj));

  check(expected, Objstr, type);
  T NewObj;
  from_string(Objstr, NewObj);
  check(expected, to_string(NewObj), "recycled " + type);
}

// There's no from_string<const char *>()...
inline void strconv(string type, const char Obj[], string expected)
{
  const string Objstr(to_string(Obj));
  check(expected, Objstr, type);
}

const double not_a_number =
#if defined(PQXX_HAVE_QUIET_NAN)
  numeric_limits<double>::quiet_NaN();
#elif defined(PQXX_HAVE_C_NAN)
  NAN;
#elif defined(PQXX_HAVE_NAN)
  nan("");
#else
  0.0/0.0;
#endif

struct intderef
{
  intderef(){}	// Silences bogus warning in some gcc versions
  template<typename ITER>
    int operator()(ITER i) const throw () { return int(*i); }
};


void test_000(transaction_base &)
{
  PQXX_CHECK_EQUAL(oid_none,
	0u,
	"InvalidIod is not zero as it used to be.  This may conceivably "
	"cause problems in libpqxx.");

  PQXX_CHECK(
	cursor_base::prior() < 0 && cursor_base::backward_all() < 0,
	"cursor_base::difference_type appears to be unsigned.");

  // Test items template
  items<int> I0;
  testitems(I0, 0);
  items<int> I1(1);
  testitems(I1, 1);
  items<int> I2(1,2);
  testitems(I2,2);
  items<int> I3(1,2,3);
  testitems(I3,3);
  items<int> I4(1,2,3,4);
  testitems(I4,4);
  items<int> I5(1,2,3,4,5);
  testitems(I5,5);
  items<int> Ivar;
  Ivar(1);
  testitems(Ivar,1);
  const string l = separated_list(",",I5.begin(),I5.end(),intderef());
  PQXX_CHECK_EQUAL(l, "1,2,3,4,5", "separated_list is broken.");
  vector<int> V2(I2);
  testitems(items<int>(V2),2);

  const char weird[] = "foo\t\n\0bar";
  const string weirdstr(weird, sizeof(weird)-1);

  // Test string conversions
  strconv("const char[]", "", "");
  strconv("const char[]", "foo", "foo");
  strconv("int", 0, "0");
  strconv("int", 100, "100");
  strconv("int", -1, "-1");

#if defined(PQXX_HAVE_LIMITS) && !defined(_MSC_VER)
  const long long_min = PGSTD::numeric_limits<long>::min(),
	long_max = PGSTD::numeric_limits<long>::max();
#else
  const long long_min = LONG_MIN, long_max = LONG_MAX;
#endif

  stringstream lminstr, lmaxstr, llminstr, llmaxstr, ullmaxstr;
#if defined(PQXX_HAVE_IMBUE)
  lminstr.imbue(locale("C"));
  lmaxstr.imbue(locale("C"));
  llminstr.imbue(locale("C"));
  llmaxstr.imbue(locale("C"));
  ullmaxstr.imbue(locale("C"));
#endif

  lminstr << long_min;
  lmaxstr << long_max;

#if defined(PQXX_HAVE_LONG_LONG)
  typedef long long longlong;
  typedef unsigned long long ulonglong;
  const unsigned long long ullong_max = ~ulonglong(0);
  const long long llong_max = longlong(ullong_max >> 2),
	llong_min = -1 - llong_max;

  llminstr << llong_min;
  llmaxstr << llong_max;
  ullmaxstr << ullong_max;
#endif

  strconv("long", 0, "0");
  strconv("long", long_min, lminstr.str());
  strconv("long", long_max, lmaxstr.str());
  strconv("double", not_a_number, "nan");
  strconv("string", string(), "");
  strconv("string", weirdstr, weirdstr);

#if defined(PQXX_HAVE_LONG_LONG)
  strconv("long long", 0LL, "0");
  strconv("long long", llong_min, llminstr.str());
  strconv("long long", llong_max, llmaxstr.str());
  strconv("unsigned long long", 0ULL, "0");
  strconv("unsigned long long", ullong_max, ullmaxstr.str());
#endif

  const char zerobuf[] = "0";
  string zero;
  from_string(zerobuf, zero, sizeof(zerobuf)-1);
  PQXX_CHECK_EQUAL(
	zero,
	zerobuf,
	"Converting \"0\" with explicit length failed.");

  const char nulbuf[] = "\0string\0with\0nuls\0";
  const string nully(nulbuf, sizeof(nulbuf)-1);
  string nully_parsed;
  from_string(nulbuf, nully_parsed, sizeof(nulbuf)-1);
  PQXX_CHECK_EQUAL(nully_parsed.size(), nully.size(), "Nul truncates string.");
  PQXX_CHECK_EQUAL(nully_parsed, nully, "String conversion breaks on nuls.");
  from_string(nully.c_str(), nully_parsed, nully.size());
  PQXX_CHECK_EQUAL(nully_parsed, nully, "Nul conversion breaks on strings.");

  stringstream ss;
  strconv("empty stringstream", ss, "");
  ss << -3.1415;
  strconv("stringstream", ss, ss.str());

  // TODO: Test binarystring reversibility

#ifdef PQXX_HAVE_PQENCRYPTPASSWORD
  const string pw = encrypt_password("foo", "bar");
  PQXX_CHECK(!pw.empty(), "Encrypting a password returned no data.");
  PQXX_CHECK_NOT_EQUAL(
	pw,
	encrypt_password("splat", "blub"), 
	"Password encryption is broken.");
  PQXX_CHECK(
	pw.find("bar") == string::npos,
	"Encrypted password contains original.");
#endif

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
    string n;
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

} // namespace

PQXX_REGISTER_TEST_NODB(test_000)
