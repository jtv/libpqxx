#include <iostream>
#include <fstream>

#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

namespace
{
bool use_tablestream=false, use_pipeline=false, use_retain=false;

const string table = "pqxxbench";

vector<string> contents;

void setup_contents(const string file)
{
  // TODO: Use input iterator
  // TODO: Restrict separator to newline
  ifstream input(file.c_str());
  string line;
  while (input >> line) contents.push_back(line);
}

void setup_table(connection_base &C, const string &file)
{
  setup_contents("pqxxbench.in");

  nontransaction T(C, "setuptable");
  
  const string create = "CREATE TEMP TABLE " + table + "("
    	"year integer, event varchar)";

  if (use_tablestream)
  {
    T.exec(create);
    tablewriter W(T, table);
    W.insert(contents.begin(), contents.end());
    W.complete();
  }
  else if (use_pipeline)
  {
    pipeline P(T);
    P.insert(create);
    if (use_retain) P.retain();
    for (vector<string>::const_iterator i=contents.begin();
	 i != contents.end();
	 ++i)
      P.insert("INSERT INTO " + table + " VALUES (" + *i + ")");
    while (!P.empty()) P.retrieve();
  }
  else
  {
    T.exec(create);
    for (vector<string>::const_iterator i=contents.begin(); 
	 i!=contents.end(); 
	 ++i)
      T.exec("INSERT INTO " + table + " VALUES (" + *i +")");
  }
  T.commit();
}

void manipulate(connection_base &C)
{
  work W(C, "manipulate");

  const string Q1 = "SELECT * FROM " + table + ", " + table + ", " + table;
  const int Q1num = 10;

  if (use_pipeline)
  {
    pipeline P(W);
    if (use_retain) P.retain();
    for (int i=0; i<Q1num; ++i) P.insert(Q1);
    for (int i=0; i<Q1num; ++i) process_q1(P.retrieve().second);
  }
  else
  {
    for (int i=0; i<Q1num; ++i) process_q1(W.exec(Q1));
  }
}

} // namespace

int main(int argc, char *argv[])
{
  try
  {
    asyncconnection C(argv[1]);
    setup_table(C, "pqxxbench.in");
  }
  catch (const broken_connection &e)
  {
    cerr << "Lost connection.  Message was: " << e.what() << endl;
    return 1;
  }
  catch (const sql_error &e)
  {
    cerr << "SQL error: " << e.what() << endl
         << "Query was: " << e.query() << endl;
    return 2;
  }
  catch (const exception &e)
  {
    cerr << e.what() << endl;
    return 2;
  }

  return 0;
}

