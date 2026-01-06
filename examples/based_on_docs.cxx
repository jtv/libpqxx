#include <iostream>
#include <pqxx/pqxx>

using namespace pqxx;

int main(int argc, char *argv[])
{
  try
  {
    pqxx::connection cx;
    std::cout << "Opened database successfully: " << cx.dbname() << '\n';
    char const *const sql = "SELECT cik from insiders";
    pqxx::nontransaction tx(cx);
    pqxx::result res(tx.exec(sql));

    for (pqxx::result::const_iterator c = res.begin(); c != res.end(); ++c)
    {
      std::cout << "CIK = " << c[0][0].view() << '\n';
    }
    std::cout << "Operation done successfully\n";
  }
  catch (const std::exception &e)
  {
    std::cerr << e.what() << '\n';
    return 1;
  }

  return 0;
}
