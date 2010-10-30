#include "test_helpers.hxx"

using namespace PGSTD;
using namespace pqxx;


namespace
{
binarystring make_binarystring(transaction_base &T, string content)
{
  return binarystring(T.exec("SELECT " + T.quote_raw(content))[0][0]);
}


void test_binarystring(transaction_base &T)
{
  binarystring b = make_binarystring(T, "");
  PQXX_CHECK(b.empty(), "Empty binarystring is not empty.");
  PQXX_CHECK_EQUAL(b.str(), "", "Empty binarystring doesn't work.");
  PQXX_CHECK_EQUAL(b.size(), 0u, "Empty binarystring has nonzero size.");
  PQXX_CHECK_EQUAL(b.length(), 0u, "Length/size mismatch.");
  PQXX_CHECK(b.begin() == b.end(), "Empty binarystring iterates.");
  PQXX_CHECK(b.rbegin() == b.rend(), "Empty binarystring reverse-iterates.");
  PQXX_CHECK_THROWS(b.at(0), out_of_range, "Empty binarystring accepts at().");

  b = make_binarystring(T, "z");
  PQXX_CHECK_EQUAL(b.str(), "z", "Basic nonempty binarystring is broken.");
  PQXX_CHECK(!b.empty(), "Nonempty binarystring is empty.");
  PQXX_CHECK_EQUAL(b.size(), 1u, "Bad binarystring size.");
  PQXX_CHECK_EQUAL(b.length(), 1u, "Length/size mismatch.");
  PQXX_CHECK(b.begin() != b.end(), "Nonempty binarystring does not iterate.");
  PQXX_CHECK(
	b.rbegin() != b.rend(),
	 "Nonempty binarystring does not reverse-iterate.");
  PQXX_CHECK(b.begin() + 1 == b.end(), "Bad iteration.");
  PQXX_CHECK(b.rbegin() + 1 == b.rend(), "Bad reverse iteration.");
  PQXX_CHECK(b.front() == 'z', "Unexpected front().");
  PQXX_CHECK(b.back() == 'z', "Unexpected back().");
  PQXX_CHECK(b.at(0) == 'z', "Unexpected data at index 0.");
  PQXX_CHECK_THROWS(b.at(1), out_of_range, "Failed to catch range error.");

  const string simple("ab");
  b = make_binarystring(T, simple);
  PQXX_CHECK_EQUAL(
	b.str(),
	simple,
	"Binary (un)escaping went wrong somewhere.");
  PQXX_CHECK_EQUAL(b.size(), simple.size(), "Escaping confuses length.");

  const string simple_escaped(T.esc_raw(simple));
  for (string::size_type i=0; i<simple_escaped.size(); ++i)
  {
    const unsigned char uc = static_cast<unsigned char>(simple_escaped[i]);
    PQXX_CHECK(uc <= 127, "Non-ASCII byte in escaped string.");
  }

  PQXX_CHECK_EQUAL(
	T.quote_raw(
		reinterpret_cast<const unsigned char *>(simple.c_str()),
		 simple.size()),
	T.quote(b),
	"quote_raw is broken");
  PQXX_CHECK_EQUAL(
	T.quote(b),
	T.quote_raw(simple),
	"Binary quoting is broken.");
  PQXX_CHECK_EQUAL(
	binarystring(T.exec("SELECT " + T.quote(b))[0][0]).str(),
	simple,
	"Binary string is not idempotent.");

  const string bytes("\x01\x23\x23\xa1\x2b\x0c\xff");
  b = make_binarystring(T, bytes);
  PQXX_CHECK_EQUAL(b.str(), bytes, "Binary data breaks (un)escaping.");
 
  const string nully("a\0b", 3);
  b = make_binarystring(T, nully);
  PQXX_CHECK_EQUAL(b.str(), nully, "Nul byte broke binary (un)escaping.");
  PQXX_CHECK_EQUAL(b.size(), 3u, "Nul byte broke binarystring size.");

  b = make_binarystring(T, "foo");
  PQXX_CHECK_EQUAL(string(b.get(), 3), "foo", "get() appears broken.");

  binarystring b1 = make_binarystring(T, "1"), b2 = make_binarystring(T, "2");
  PQXX_CHECK_NOT_EQUAL(b1.get(), b2.get(), "Madness rules.");
  PQXX_CHECK_NOT_EQUAL(b1.str(), b2.str(), "Logic has no more meaning.");
  b1.swap(b2);
  PQXX_CHECK_NOT_EQUAL(b1.str(), b2.str(), "swap() equalized binarystrings.");
  PQXX_CHECK_NOT_EQUAL(b1.str(), "1", "swap() did not happen.");
  PQXX_CHECK_EQUAL(b1.str(), "2", "swap() is broken.");
  PQXX_CHECK_EQUAL(b2.str(), "1", "swap() went insane.");

  b = make_binarystring(T, "bar");
  b.swap(b);
  PQXX_CHECK_EQUAL(b.str(), "bar", "Self-swap confuses binarystring.");

  b = make_binarystring(T, "\\x");
  PQXX_CHECK_EQUAL(b.str(), "\\x", "Hex-escape header confused (un)escaping.");
}
} // namespace

PQXX_REGISTER_TEST(test_binarystring)
