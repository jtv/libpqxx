#include "pqxx/compiler.h"

#include <iostream>

#include <pqxx/pqxx>

using namespace PGSTD;
using namespace pqxx;


// Initial test program for libpqxx.  Test functionality that doesn't require a
// running database.
//
// Usage: test000

namespace
{
template<typename T> void testitems(const T &I, typename T::size_type s)
{
  if (I.size() != s)
    throw logic_error("Error in items class: expected " + to_string(s) + " "
	"items, got " + to_string(I.size()));
  for ( ; s; --s)
    if (typename T::size_type(I[s-1]) != s)
      throw logic_error("Found value " + to_string(I[s-1]) + " in items "
	  "where " + to_string(s) + " was expected");
}

void check(string ref, string val, string vdesc)
{
  if (ref != val)
    throw logic_error("String mismatch: (" + vdesc + ") '" + val + "' "
	"<> " + ref + "'");
}

void esc(string str, string expected=string())
{
  if (expected.empty()) expected = str;
  check(expected, sqlesc(str), "string");

  if (str.size() < strlen(str.c_str()))
  {
    check(expected, sqlesc(str.c_str()), "const char[]");
    check(expected, sqlesc(str.c_str(), str.size()), "const char[],size_t");
    check(expected, sqlesc(str.c_str(),
	  strlen(str.c_str())), "const char[],strlen(...)");
    check(expected, sqlesc(str.c_str(),10000), "const char[],1000");
  }
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


} // namespace

int main()
{
  try
  {
    if (oid_none)
      throw logic_error("InvalidOid is not zero as it used to be."
	  "This may conceivably cause problems in libpqxx." );

    if (cursor_base::prior() >= 0 || cursor_base::backward_all() >= 0)
      throw logic_error("cursor_base::difference_type appears to be unsigned");

    cout << "Testing items template..." << endl;
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
    vector<int> V2(I2);
    testitems(items<int>(V2),2);

    const char weird[] = "foo\t\0bar";
    const string weirdstr(weird, sizeof(weird)-1);

    cout << "Testing SQL string escape functions..." << endl;
    esc("");
    esc("foo");
    esc("foo bar");
    esc("unquote' ha!", "unquote'' ha!");
    esc("'", "''");
    esc("\\", "\\\\");
    esc("\t");
    esc(weirdstr, "foo\t\\000bar");

    cout << "Testing string conversions..." << endl;
    strconv("const char[]", "", "");
    strconv("const char[]", "foo", "foo");
    strconv("int", 0, "0");
    strconv("int", 100, "100");
    strconv("int", -1, "-1");

    const long long_min = PGSTD::numeric_limits<long>::min(),
	       long_max = PGSTD::numeric_limits<long>::max();

    strconv("long", long_min, to_string(long_min));
    strconv("long", long_max, to_string(long_max));
    strconv("double", 0.0, "0");
    strconv("string", string(), "");
    strconv("string", weirdstr, weirdstr);

    stringstream ss;
    strconv("empty stringstream", ss, "");
    ss << -3.1415;
    strconv("stringstream", ss, ss.str());

    // TODO: Test binarystring reversibility

    // Test error handling for failed connections
    try
    {
      nullconnection nc;
      work w(nc);
      throw logic_error("nullconnection failed to fail!");
    }
    catch (broken_connection &c)
    {
      cout << "(Expected) " << c.what() << endl;
    }
    try
    {
      nullconnection nc("");
      work w(nc);
      throw logic_error("nullconnection(const char[]) failed to fail!");
    }
    catch (broken_connection &c)
    {
      cout << "(Expected) " << c.what() << endl;
    }
    try
    {
      string n;
      nullconnection nc(n);
      work w(nc);
      throw logic_error("nullconnection(const std::string &) failed to fail!");
    }
    catch (broken_connection &c)
    {
      cout << "(Expected) " << c.what() << endl;
    }
  }
  catch (const bad_alloc &)
  {
    cerr << "Out of memory!" << endl;
  }
  catch (const exception &e)
  {
    // All exceptions thrown by libpqxx are derived from std::exception
    cerr << "Exception: " << e.what() << endl;
    return 2;
  }
  catch (...)
  {
    // This is really unexpected (see above)
    cerr << "Unhandled exception" << endl;
    return 100;
  }

  return 0;
}


