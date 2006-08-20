#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/connection>
#include <pqxx/cursor>
#include <pqxx/transaction>
#include <pqxx/result>

using namespace PGSTD;
using namespace pqxx;


// "Adopted SQL cursor" test program for libpqxx.  Create SQL cursor, wrap it in
// a cursor object.  Then scroll it back and forth and check for consistent 
// results.
//
// Usage: test045 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{

void AddResult(vector<string> &V, const result &R)
{
  V.reserve(V.size() + R.size());
  for (result::const_iterator i = R.begin(); i != R.end(); ++i)
    V.push_back(i.at(0).c_str());
}


void DumpRows(const vector<string> &V)
{
  for (vector<string>::const_iterator i = V.begin(); i != V.end(); ++i)
    cout << "\t" << *i << endl;
  cout << endl;
}

} // namespace


int main(int, char *argv[])
{
  try
  {
    const string Table = "pqxxevents";

    connection C(argv[1]);
    transaction<serializable> T(C, "test45");

    // Count rows.
    result R( T.exec("SELECT count(*) FROM " + Table) );

    if (R.at(0).at(0).as<long>() <= 10) 
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    if (R[0][0].as<int>() != int(R[0][0].as<unsigned int>()))
      throw runtime_error("Are there really that many rows in " + Table + "?");

    int i;
    from_string(R[0][0].c_str(), i);
    if (i != R[0][0].as<int>())
      throw runtime_error("from_string() yielded " + to_string(i) + " "
	  "for '" + R[0][0].c_str() + "'");
    from_string(string(R[0][0].c_str()), i);
    if (i != R[0][0].as<int>())
      throw runtime_error("from_string(const char[],int &) "
	  "inconsistent with from_string(const std::string &,int &)");

    unsigned int ui;
    from_string(R[0][0].c_str(), ui);
    if (int(ui) != i)
      throw runtime_error("from_string() yields different unsigned int");

    long l;
    from_string(R[0][0].c_str(), l);
    if (i != l)
      throw runtime_error("from_string() yields int that differs from long");

    unsigned long ul;
    from_string(R[0][0].c_str(), ul);
    if (long(ui) != l)
      throw runtime_error("from_string() yields different unsigned long");

    short s;
    from_string(R[0][0].c_str(), s);
    if (s != i)
      throw runtime_error("from_string() yields different short");

    unsigned short us;
    from_string(R[0][0].c_str(), us);
    if (us != ui)
      throw runtime_error("from_string() yields different unsigned short");

    // Create an SQL cursor and, for good measure, muddle up its state a bit.
    const string CurName = "mycur";
    T.exec("DECLARE " + CurName + " CURSOR FOR SELECT * FROM " + Table);
    T.exec("MOVE ALL IN " + CurName);

    int GetRows = 3;

    // Wrap cursor in cursor object.
    cursor Cur(&T, CurName);

    // Reset Cur to the beginning of our result set so that it may know its
    // position.
    Cur.move(cursor_base::backward_all());

    // Now start testing our new cursor.
    cursor::difference_type offset;
    R = Cur.fetch(GetRows, offset);
    if (offset > GetRows)
      throw logic_error("Fetched " + to_string(GetRows) + " rows, "
      	"but displacement was " + to_string(offset));
    if (result::size_type(offset) != R.size())
      throw logic_error("Received " + to_string(R.size()) + " rows, "
	"but displacement was " + to_string(offset));

    if (R.size() > result::size_type(GetRows))
      throw logic_error("Expected " + to_string(GetRows) + " rows, "
		        "got " + to_string(R.size()));

    if (R.size() < result::size_type(GetRows))
      cerr << "Warning: asked for " << GetRows << " rows, "
	      "got only " << R.size() << endl;

    // Remember those first rows...
    vector<string> FirstRows1;
    AddResult(FirstRows1, R);

    // Now add one more
    R = Cur.fetch(1);
    if (R.size() != 1)
      throw logic_error("Asked for 1 row, got " + to_string(R.size()));
    AddResult(FirstRows1, R);

    R = Cur.fetch(GetRows);
    if (R.size() != result::size_type(GetRows))
      throw logic_error("Asked for " + to_string(GetRows) + " rows, "
		        "got " + to_string(R.size()) + ". "
			"Looks like Fetch() changed our cursor's stride!");
    AddResult(FirstRows1, R);

    // Dump current contents of FirstRows1
    cout << "First rows are:" << endl;
    DumpRows(FirstRows1);

    // Move cursor 1 step forward to make subsequent backwards fetch include
    // current row
    Cur.move(cursor_base::next());

    // Fetch the same rows we just fetched into FirstRows1, but backwards
    R = Cur.fetch(cursor_base::backward_all());

    vector<string> FirstRows2;
    AddResult(FirstRows2, R);

    cout << "First rows read backwards are:" << endl;
    DumpRows(FirstRows2);

    if (R.size() != FirstRows1.size())
      throw logic_error("I read " + to_string(FirstRows1.size()) + " rows, "
		        "but I see " + to_string(R.size()) + " rows "
			"when trying to read them backwards!");

    sort(FirstRows1.begin(), FirstRows1.end());
    sort(FirstRows2.begin(), FirstRows2.end());
    if (FirstRows1 != FirstRows2)
      throw logic_error("First rows are not the same read backwards "
		        "as they were read forwards!");

    R = Cur.fetch(cursor_base::next());
    if (R.size() != 1) 
      throw logic_error("NEXT: wanted 1 row, got " + to_string(R.size()));
    const string Row = R[0][0].c_str();

    Cur.move(3);
    Cur.move(-2);

    R = Cur.fetch(cursor_base::prior());
    if (R.size() != 1)
      throw logic_error("PRIOR: wanted 1 row, got " + to_string(R.size()));

    if (R[0][0].c_str() != Row)
      throw logic_error("First row was '" + Row + "' going forward, "
		        "but '" + R[0][0].c_str() + "' going back!");
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

