#include <iostream>
#include <pqxx/pqxx>

using namespace std;
using namespace pqxx;

int main(int argc, char *argv[])
{
  const char *sql;

  try
  {
    connection C;
    cout << "Opened database successfully: " << C.dbname() << endl;
    sql = "SELECT cik from insiders";
    nontransaction N(C);
    result R(N.exec(sql));

    for (result::const_iterator c = R.begin(); c != R.end(); ++c)
    {
      cout << "CIK = " << c[0][0].as<string>() << endl;
    }
    cout << "Operation done successfully" << endl;
  }
  catch (const std::exception &e)
  {
    cerr << e.what() << std::endl;
    return 1;
  }

  return 0;
}
