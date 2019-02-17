#include "../test_helpers.hxx"

using namespace pqxx;


namespace
{
binarystring make_binarystring(transaction_base &T, std::string content)
{
  return binarystring(T.exec("SELECT " + T.quote_raw(content))[0][0]);
}


void test_binarystring()
{
  connection conn;
  work tx{conn};
  binarystring b = make_binarystring(tx, "");
  PQXX_CHECK(b.empty(), "Empty binarystring is not empty.");
  PQXX_CHECK_EQUAL(b.str(), "", "Empty binarystring doesn't work.");
  PQXX_CHECK_EQUAL(b.size(), 0u, "Empty binarystring has nonzero size.");
  PQXX_CHECK_EQUAL(b.length(), 0u, "Length/size mismatch.");
  PQXX_CHECK(b.begin() == b.end(), "Empty binarystring iterates.");
  PQXX_CHECK(b.cbegin() == b.begin(), "Wrong cbegin for empty binarystring.");
  PQXX_CHECK(b.rbegin() == b.rend(), "Empty binarystring reverse-iterates.");
  PQXX_CHECK(
	b.crbegin() == b.rbegin(),
	"Wrong crbegin for empty binarystring.");
  PQXX_CHECK_THROWS(
	b.at(0),
	std::out_of_range,
	"Empty binarystring accepts at().");

  b = make_binarystring(tx, "z");
  PQXX_CHECK_EQUAL(b.str(), "z", "Basic nonempty binarystring is broken.");
  PQXX_CHECK(not b.empty(), "Nonempty binarystring is empty.");
  PQXX_CHECK_EQUAL(b.size(), 1u, "Bad binarystring size.");
  PQXX_CHECK_EQUAL(b.length(), 1u, "Length/size mismatch.");
  PQXX_CHECK(b.begin() != b.end(), "Nonempty binarystring does not iterate.");
  PQXX_CHECK(
	b.rbegin() != b.rend(),
	 "Nonempty binarystring does not reverse-iterate.");
  PQXX_CHECK(b.begin() + 1 == b.end(), "Bad iteration.");
  PQXX_CHECK(b.rbegin() + 1 == b.rend(), "Bad reverse iteration.");
  PQXX_CHECK(b.cbegin() == b.begin(), "Wrong cbegin.");
  PQXX_CHECK(b.cend() == b.end(), "Wrong cend.");
  PQXX_CHECK(b.crbegin() == b.rbegin(), "Wrong crbegin.");
  PQXX_CHECK(b.crend() == b.rend(), "Wrong crend.");
  PQXX_CHECK(b.front() == 'z', "Unexpected front().");
  PQXX_CHECK(b.back() == 'z', "Unexpected back().");
  PQXX_CHECK(b.at(0) == 'z', "Unexpected data at index 0.");
  PQXX_CHECK_THROWS(b.at(1), std::out_of_range, "Failed to catch range error.");

  const std::string simple("ab");
  b = make_binarystring(tx, simple);
  PQXX_CHECK_EQUAL(
	b.str(),
	simple,
	"Binary (un)escaping went wrong somewhere.");
  PQXX_CHECK_EQUAL(b.size(), simple.size(), "Escaping confuses length.");

  const std::string simple_escaped(tx.esc_raw(simple));
  for (std::string::size_type i=0; i<simple_escaped.size(); ++i)
  {
    const unsigned char uc = static_cast<unsigned char>(simple_escaped[i]);
    PQXX_CHECK(uc <= 127, "Non-ASCII byte in escaped string.");
  }

  PQXX_CHECK_EQUAL(
	tx.quote_raw(
		reinterpret_cast<const unsigned char *>(simple.c_str()),
		 simple.size()),
	tx.quote(b),
	"quote_raw is broken");
  PQXX_CHECK_EQUAL(
	tx.quote(b),
	tx.quote_raw(simple),
	"Binary quoting is broken.");
  PQXX_CHECK_EQUAL(
	binarystring(tx.exec1("SELECT " + tx.quote(b))[0]).str(),
	simple,
	"Binary string is not idempotent.");

  const std::string bytes("\x01\x23\x23\xa1\x2b\x0c\xff");
  b = make_binarystring(tx, bytes);
  PQXX_CHECK_EQUAL(b.str(), bytes, "Binary data breaks (un)escaping.");

  const std::string nully("a\0b", 3);
  b = make_binarystring(tx, nully);
  PQXX_CHECK_EQUAL(b.str(), nully, "Nul byte broke binary (un)escaping.");
  PQXX_CHECK_EQUAL(b.size(), 3u, "Nul byte broke binarystring size.");

  b = make_binarystring(tx, "foo");
  PQXX_CHECK_EQUAL(std::string(b.get(), 3), "foo", "get() appears broken.");

  binarystring b1 = make_binarystring(tx, "1"), b2 = make_binarystring(tx, "2");
  PQXX_CHECK_NOT_EQUAL(b1.get(), b2.get(), "Madness rules.");
  PQXX_CHECK_NOT_EQUAL(b1.str(), b2.str(), "Logic has no more meaning.");
  b1.swap(b2);
  PQXX_CHECK_NOT_EQUAL(b1.str(), b2.str(), "swap() equalized binarystrings.");
  PQXX_CHECK_NOT_EQUAL(b1.str(), "1", "swap() did not happen.");
  PQXX_CHECK_EQUAL(b1.str(), "2", "swap() is broken.");
  PQXX_CHECK_EQUAL(b2.str(), "1", "swap() went insane.");

  b = make_binarystring(tx, "bar");
  b.swap(b);
  PQXX_CHECK_EQUAL(b.str(), "bar", "Self-swap confuses binarystring.");

  b = make_binarystring(tx, "\\x");
  PQXX_CHECK_EQUAL(b.str(), "\\x", "Hex-escape header confused (un)escaping.");
}


PQXX_REGISTER_TEST(test_binarystring);
} // namespace
