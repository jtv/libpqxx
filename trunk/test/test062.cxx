#include <cstring>
#include <iostream>

#include "test_helpers.hxx"


using namespace std;
using namespace pqxx;


// Example program for libpqxx.  Test binarystring functionality.
namespace
{
void test_062(transaction_base &T)
{
  const string TestStr =
	"Nasty\n\030Test\n\t String with \200\277 weird bytes "
	"\r\0 and Trailer\\\\\0";

  T.exec("CREATE TEMP TABLE pqxxbin (binfield bytea)");

  const string Esc = T.esc_raw(TestStr),
	Chk = T.esc_raw(reinterpret_cast<const unsigned char *>(TestStr.c_str()),
                      strlen(TestStr.c_str()));

  PQXX_CHECK_EQUAL(Chk, Esc, "Inconsistent results from esc_raw().");

  T.exec("INSERT INTO pqxxbin VALUES ('" + Esc + "')");

  result R = T.exec("SELECT * from pqxxbin");
  T.exec("DELETE FROM pqxxbin");

  binarystring B( R.at(0).at(0) );

  PQXX_CHECK(!B.empty(), "Binary string became empty in conversion.");

  PQXX_CHECK_EQUAL(B.size(), TestStr.size(), "Binary string was mangled.");

  binarystring::const_iterator c;
  binarystring::size_type i;
  for (i=0, c=B.begin(); i<B.size(); ++i, ++c)
  {
    PQXX_CHECK(c != B.end(), "Premature end to binary string.");

    const char x = TestStr.at(i), y = char(B.at(i)), z = char(B.data()[i]);

    PQXX_CHECK_EQUAL(
	string(&x, 1),
	string(&y, 1),
	"Binary string byte changed.");

    PQXX_CHECK_EQUAL(
	string(&y, 1),
	string(&z, 1),
	"Inconsistent byte at offset " + to_string(i) + ".");
  }

  PQXX_CHECK(B.at(0) == B.front(), "front() is inconsistent with at(0).");
  PQXX_CHECK(c == B.end(), "end() of binary string not reached.");

  --c;

  PQXX_CHECK(*c == B.back(), "binarystring::back() is broken.");

  binarystring::const_reverse_iterator r;
  for (i=B.length(), r=B.rbegin(); i>0; --i, ++r)
  {
    PQXX_CHECK(
	r != B.rend(),
	"Premature rend to binary string at " + to_string(i) + ".");

    typedef unsigned char uchar;
    PQXX_CHECK_EQUAL(
	int(B[i-1]),
	int(uchar(TestStr.at(i-1))),
	"Reverse iterator is broken.");
  }
  PQXX_CHECK(r == B.rend(), "rend() of binary string not reached.");

  PQXX_CHECK_EQUAL(B.str(), TestStr, "Binary string was mangled.");

  const string TestStr2("(More conventional text)");
  T.exec("INSERT INTO pqxxbin VALUES ('" + TestStr2 + "')");
  R = T.exec("SELECT * FROM pqxxbin");
  binarystring B2(R.front().front());

  PQXX_CHECK(!(B2 == B), "False positive on binarystring::operator==().");
  PQXX_CHECK(B2 != B, "False negative on binarystring::operator!=().");

  binarystring B1c(B), B2c(B2);
  PQXX_CHECK(!(B1c != B), "Copied binarystring differs from original.");
  PQXX_CHECK(B2c == B2, "Copied binarystring not equal to original.");

  B1c.swap(B2c);

  PQXX_CHECK(B2c != B1c, "binarystring::swap() produced identical strings.");
  PQXX_CHECK(B2c == B, "binarystring::swap() is broken.");
  PQXX_CHECK(B1c == B2, "Cross-check of swapped binarystrings failed.");
}
} // namespace

PQXX_REGISTER_TEST(test_062)
