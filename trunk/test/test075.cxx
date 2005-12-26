#include <pqxx/compiler.h>

#include <iostream>
#include <vector>

#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// Test program for libpqxx.  Compare const_reverse_iterator iteration of a
// result to a regular, const_iterator iteration.
//
// Usage: test075 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, or "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

int main(int, char *argv[])
{
  try
  {
    connection C(argv[1]);
    work W(C, "test75");
    const result R( W.exec("SELECT year FROM pqxxevents") );

    if (R.empty()) throw runtime_error("No events found, can't test!");

    if (!(R[0] == R.at(0)))
      throw logic_error("result[0] == result.at(0) doesn't hold!");
    if (R[0] != R.at(0))
      throw logic_error("Something wrong with result::tuple::operator!=");

    if (!(R[0][0] == R[0].at(0)))
      throw logic_error("tuple[0] == tuple.at(0) doesn't hold!");
    if (R[0][0] != R[0].at(0))
      throw logic_error("Something wrong with result::field::operator!=");

    vector<string> contents;
    for (result::const_iterator i=R.begin(); i!=R.end(); ++i)
      contents.push_back(i->at(0).as<string>());
    cout << to_string(contents.size()) << " years read" << endl;

    if (contents.size() != vector<string>::size_type(R.size()))
      throw logic_error("Got " + to_string(contents.size()) + " values "
	  "out of result with size " + to_string(R.size()));

    for (result::size_type i=0; i<R.size(); ++i)
      if (contents[i] != R.at(i).at(0).c_str())
	throw logic_error("Inconsistent iteration: '" + contents[i] + "' "
	    "became '" + R[i][0].as<string>());
    cout << to_string(R.size()) << " years checked" << endl;

#ifdef PQXX_HAVE_REVERSE_ITERATOR
    // Thorough test for result::const_reverse_iterator
    result::const_reverse_iterator ri1(R.rbegin()), ri2(ri1), ri3(R.end());
    ri2 = R.rbegin();

    if (!(ri1 == ri2))
      throw logic_error("Copy-constructed reverse_iterator "
	  "not identical to assigned one");
    if (ri2 != ri3)
      throw logic_error("result:end() does not generate rbegin()");
    if (ri2 - ri3)
      throw logic_error("Distance between identical const_reverse_iterators "
	  "is nonzero: " + to_string(ri2 - ri3));
    if (ri2 != ri3 + 0)
      throw logic_error("reverse_iterator+0 gives strange result");
    if (ri2 != ri3 - 0)
      throw logic_error("reverse_iterator-0 gives strange result");
    if (ri3 < ri2)
      throw logic_error("Equality with reverse_iterator operator < wrong");
    if (!(ri2 <= ri3))
      throw logic_error("Equality with reverse_iterator operator <= wrong");

    if (ri3++ != ri2)
      throw logic_error("reverse_iterator postfix ++ returns wrong result");

    if (ri3 - ri2 != 1)
      throw logic_error("Nonzero reverse_iterator distance came out at " +
	  to_string(ri3 - ri2) + ", "
	  "expected 1");
    if (!(ri3 > ri2))
      throw logic_error("Something wrong with reverse_iterator operator >");
    if (!(ri3 >= ri2))
      throw logic_error("Something wrong with reverse_iterator operator >=");
    if (!(ri2 < ri3))
      throw logic_error("Something wrong with reverse_iterator operator <");
    if (!(ri2 <= ri3))
      throw logic_error("Something wrong with reverse_iterator operator <=");
    if (ri3 != ri2 + 1)
      throw logic_error("Adding number to reverse_iterator goes wrong");
    if (ri2 != ri3 - 1)
      throw logic_error("Subtracting from reverse_iterator goes wrong");

    if (ri3 != ++ri2)
      throw logic_error("reverse_iterator prefix ++ returns wrong result");
    if (!(ri3 >= ri2))
      throw logic_error("Equality with reverse_iterator operator >= failed");
    if (!(ri3 >= ri2))
      throw logic_error("Equality with reverse_iterator operator <= failed");
    if (ri3.base() != R.back())
      throw logic_error("reverse_iterator does not arrive at back()");
    if (ri1->at(0) != (*ri1).at(0))
      throw logic_error("reverse_iterator -> differs from * operator");

    if (ri2-- != ri3)
      throw logic_error("reverse_iterator postfix -- returns wrong result");
    if (ri2 != --ri3)
      throw logic_error("reverse_iterator prefix -- returns wrong result");

    if (ri2 != R.rbegin())
      throw logic_error("Something wrong with reverse_iterator -- operator");

    ri2 += 1;
    ri3 -= -1;

    if (ri2 == R.rbegin())
      throw logic_error("Adding to reverse_iterator doesn't work");
    if (ri3 != ri2)
      throw logic_error("reverse_iterator -= broken for negative numbers?");

    ri2 -= 1;
    if (ri2 != R.rbegin())
      throw logic_error("reverse_iterator += and -= do not cancel out");


    // Now verify that reverse iterator also sees the same results...
    vector<string>::reverse_iterator l = contents.rbegin();
    for (result::const_reverse_iterator i = R.rbegin(); i != R.rend(); ++i, ++l)
    {
      if (*l != i->at(0).c_str())
	throw logic_error("Inconsistent reverse iteration: "
	    "'" + *l + "' became '" + (*i)[0].as<string>() + "'");
    }

    if (l != contents.rend())
      throw logic_error("Reverse iteration of result ended too soon");
#endif	// PQXX_HAVE_REVERSE_ITERATOR

    if (R.empty())
      throw runtime_error("No years found in events table, can't test!");
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: '" << e.query() << "'" << endl;
    return 1;
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

