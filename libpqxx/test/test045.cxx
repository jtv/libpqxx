#include <algorithm>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include <pqxx/connection.h>
#include <pqxx/cursor.h>
#include <pqxx/transaction.h>
#include <pqxx/result.h>

using namespace PGSTD;
using namespace pqxx;


// "Adopted SQL Cursor" test program for libpqxx.  Create SQL cursor, wrap it in
// a Cursor object.  Then scroll it back and forth and check for consistent 
// results.
//
// Usage: test45 [connect-string]
//
// Where connect-string is a set of connection options in Postgresql's
// PQconnectdb() format, eg. "dbname=template1" to select from a database
// called template1, of "host=foo.bar.net user=smith" to connect to a
// backend running on host foo.bar.net, logging in as user smith.

namespace
{

void AddResult(vector<string> &V, const Result &R)
{
  V.reserve(V.size() + R.size());
  for (Result::const_iterator i = R.begin(); i != R.end(); ++i)
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
    const string Table = "events";

    Connection C(argv[1]);
    Transaction T(C, "test45");

    // Count rows.
    Result R( T.Exec("SELECT count(*) FROM " + Table) );
    int Rows;
    R.at(0).at(0).to(Rows);

    if (Rows <= 10) 
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    // Create an SQL cursor and, for good measure, muddle up its state a bit.
    const string CurName = "MYCUR";
    T.Exec("DECLARE " + CurName + " CURSOR FOR SELECT * FROM " + Table);
    T.Exec("MOVE ALL IN " + CurName);

    int GetRows = 3;

    // Wrap cursor in Cursor object.  Apply some trickery to get its name inside
    // a result field for this purpose.  This isn't easy because it's not 
    // supposed to be easy; normally we'd only construct Cursors around existing
    // SQL cursors if they were being returned by functions.
    Cursor Cur(T, T.Exec("SELECT " + Quote(CurName))[0][0], GetRows);

    // Reset Cur to the beginning of our result set so that it may know its
    // position.
    Cur.Move(Cursor::BACKWARD_ALL());

    // Now start testing our new Cursor.
    Cur >> R;

    if (R.size() > GetRows)
      throw logic_error("Expected " + ToString(GetRows) + " rows, "
		        "got " + ToString(R.size()));

    if (R.size() < GetRows)
      cerr << "Warning: asked for " << GetRows << " rows, "
	      "got only " << R.size() << endl;

    // Remember those first rows...
    vector<string> FirstRows1;
    AddResult(FirstRows1, R);

    // Now add one more
    R = Cur.Fetch(1);
    if (R.size() != 1)
      throw logic_error("Asked for 1 row, got " + ToString(R.size()));
    AddResult(FirstRows1, R);

    // Now see if that Fetch() didn't confuse our cursor's stride
    Cur >> R;
    if (R.size() != GetRows)
      throw logic_error("Asked for " + ToString(GetRows) + " rows, "
		        "got " + ToString(R.size()) + ". "
			"Looks like Fetch() changed our cursor's stride!");
    AddResult(FirstRows1, R);

    // Dump current contents of FirstRows1
    cout << "First rows are:" << endl;
    DumpRows(FirstRows1);

    // Move cursor 1 step forward to make subsequent backwards fetch include
    // current row
    Cur += 1;

    // Fetch the same rows we just fetched into FirstRows1, but backwards
    Cur.SetCount(Cursor::BACKWARD_ALL());
    Cur >> R;

    vector<string> FirstRows2;
    AddResult(FirstRows2, R);

    cout << "First rows read backwards are:" << endl;
    DumpRows(FirstRows2);

    if (vector<string>::size_type(R.size()) != FirstRows1.size())
      throw logic_error("I read " + ToString(FirstRows1.size()) + " rows, "
		        "but I see " + ToString(R.size()) + " rows "
			"when trying to read them backwards!");

    sort(FirstRows1.begin(), FirstRows1.end());
    sort(FirstRows2.begin(), FirstRows2.end());
    if (FirstRows1 != FirstRows2)
      throw logic_error("First rows are not the same read backwards "
		        "as they were read forwards!");

    R = Cur.Fetch(Cursor::NEXT());
    if (R.size() != 1) 
      throw logic_error("NEXT: wanted 1 row, got " + ToString(R.size()));
    const string Row = R[0][0].c_str();

    Cur += 3;
    Cur -= 2;

    R = Cur.Fetch(Cursor::PRIOR());
    if (R.size() != 1)
      throw logic_error("PRIOR: wanted 1 row, got " + ToString(R.size()));

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

