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


// Cursor test program for libpqxx.  Read table through a cursor, scanning back 
// and forth and checking for consistent results.
//
// Usage: test019 [connect-string]
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
    transaction<serializable> T(C, "test19");

    // Count rows.
    result R( T.exec("SELECT count(*) FROM " + Table) );
    int Rows;
    R.at(0).at(0).to(Rows);

    if (Rows <= 10) 
      throw runtime_error("Not enough rows in '" + Table + "' "
		          "for serious testing.  Sorry.");

    int GetRows = 3;
    cursor Cur(&T, "SELECT * FROM " + Table, "tablecur" );
    R = Cur.fetch(GetRows);

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

    // Now see if that Fetch() didn't confuse our cursor's stride
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
    Cur.move(1);

    // Fetch the same rows we just fetched into FirstRows1, but backwards
    R = Cur.fetch(cursor_base::backward_all());

    vector<string> FirstRows2;
    AddResult(FirstRows2, R);

    cout << "First rows read backwards are:" << endl;
    DumpRows(FirstRows2);

    if (vector<string>::size_type(R.size()) != FirstRows1.size())
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
    // If we're interested in the text of a failed query, we can write separate
    // exception handling code for this type of exception
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

