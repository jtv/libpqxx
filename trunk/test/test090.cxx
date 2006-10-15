#include <cassert>
#include <iostream>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;

namespace
{
void check(string ref, string val, string vdesc)
{
  if (ref != val)
    throw logic_error("String mismatch: (" + vdesc + ") '" + val + "' "
	"<> " + ref + "'");
}


void esc(transaction_base &t, string str, string expected=string())
{
  if (expected.empty()) expected = str;
  check(expected, t.esc(str), "string");
  check(expected, t.esc(str.c_str()), "const char[]");
  check(expected, t.esc(str.c_str(), str.size()), "const char[],size_t");
  check(expected, t.esc(str.c_str(), 1000), "const char[],1000");
}


void esc_raw(transaction_base &t, string str, string expected=string())
{
  if (expected.empty()) expected = str;
  const unsigned char *c = reinterpret_cast<const unsigned char *>(str.c_str());
  check(expected, t.esc_raw(str), "string");
  check(expected, t.esc_raw(c, str.size()), "const unsigned char[],size_t");
}

void esc_both(transaction_base &t, string str, string expected=string())
{
  esc(t, str, expected);
  esc_raw(t, str, expected);
}

void dotests(transaction_base &t)
{
  esc_both(t, "");
  esc_both(t, "foo");
  esc_both(t, "foo bar");
  esc_both(t, "unquote' ha!", "unquote'' ha!");
  esc_both(t, "'", "''");
  esc(t, "\\", "\\\\");
  esc(t, "\t");

  const char weird[] = "foo\t\n\0bar";
  const string weirdstr(weird, sizeof(weird)-1);
  esc_raw(t, weirdstr, "foo\\\\011\\\\012\\\\000bar");
}
} // namespace


// Test program for libpqxx.  Test string-escaping functions.
//
// Usage: test090
int main()
{
  try
  {
    connection C;

    // Test connection's adorn_name() function for uniqueness
    const string nametest = "basename";
    const string nt1 = C.adorn_name(nametest),
                 nt2 = C.adorn_name(nametest);
    if (nt1 == nt2)
      throw logic_error("\"Unique\" names are not unique: got '" + nt1 + "' "
	"twice");

    nontransaction N(C, "test90non");
    dotests(N);
    N.abort();

    work W(C, "test90work");
    dotests(W);
    W.abort();
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 1;
  }
  catch (const exception &e)
  {
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


