#include <pqxx/compiler.h>

#include <iostream>

#include <pqxx/connection>
#include <pqxx/nontransaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Read and print table using field iterators.
//
// Usage: test082 [table] [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    connection C(argv[1] ? argv[2] : 0);
    nontransaction T(C, "test82");
    const string Table = (argv[1] ? argv[1] : "pqxxevents");
    result R( T.exec("SELECT * FROM " + Table) );
    C.disconnect();

    if (R.empty()) throw runtime_error("Got empty result!");

    const string nullstr("[null]");

    for (result::tuple::const_iterator f = R[0].begin(); f != R[0].end(); ++f)
      cout << f->name() << '\t';
    cout << endl << endl;
    for (result::const_iterator r = R.begin(); r != R.end(); ++r)
    {
      result::tuple::const_iterator f2(r[0]);
      for (result::tuple::const_iterator f=r->begin(); f!=r->end(); ++f, f2++)
      {
	cout << f->c_str() << '\t';
	if ((*f2).as(nullstr) != f->as(nullstr))
	  throw logic_error("Inconsistent iteration result: "
	      "'" + (*f2).as<string>() + "' vs '" + f2->c_str() + "'");
      }

      if (r->begin() + r->size() != r->end())
	throw logic_error("Tuple end() appears to be in the wrong place");
      if (r->size() + r->begin() != r->end())
	throw logic_error("Field iterator addition not commutative");
      if (r->begin()->num() != 0)
	throw logic_error("Unexpected column number at begin(): " +
	    to_string(r->begin()->num()));

      result::tuple::const_iterator f3(r[r->size()]);
      if (!(f3 == r->end()))
	throw logic_error("Did not get end() at end of tuple");
      if (f3 <= r->begin())
	throw logic_error("Tuple end() appears to precede tuple begin()");
      if (f3 < r->end() || !(r->begin() < f3))
	throw logic_error("Field iterator < operator seems to be broken");
      if (!(f3 > r->begin()))
	throw logic_error("Tuple end() not greater than begin(); empty tuple?");
      result::tuple::const_iterator f4(*r, r->size());
      if (f4 != f3)
	throw logic_error("Field iterator constructor with offset broken");

      f3--;
      f4 -= 1;

      if (!(f3 < r->end()))
	throw logic_error("Last field in tuple not before end()");
      if (!(f3 >= r.begin()))
	throw logic_error("Last field in tuple appears to precede begin()");
      if (f3 != r.end()-1)
	throw logic_error("Back from end() does not  yield end()-1");
      if (r->end() - f3 != 1)
	throw logic_error("Wrong distance from last tuple to end(): "
	    "expected 1, got " + to_string(r->end() - f3));
      if (f4 != f3)
	throw logic_error("Looks like field iterator -= doesn't work");
      f4 += 1;
      if (f4 != r->end())
	throw logic_error("Looks like field iterator += doesn't work");

#ifdef PQXX_HAVE_REVERSE_ITERATOR
      for (result::tuple::const_reverse_iterator fr = r->rbegin();
	   fr != r->rend();
	   ++fr, --f3)
      {
      }
#endif	// PQXX_HAVE_REVERSE_ITERATOR

      cout <<endl;
    }
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
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

