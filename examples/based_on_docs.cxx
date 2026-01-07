#include <iostream>
#include <pqxx/pqxx>

namespace
{
void set_up(pqxx::connection &cx)
{
  pqxx::work tx{cx};
  tx.exec("CREATE TEMP TABLE insiders(cik varchar)");
  tx.exec("INSERT INTO insiders(cik) VALUES ('82a3764f'), ('359b0625')");
  tx.commit();
}
} // namespace


int main(int, char **)
{
  try
  {
    pqxx::connection cx;
    std::cout << "Opened database successfully: " << cx.dbname() << '\n';

    set_up(cx);

    char const *const sql = "SELECT cik from insiders";
    pqxx::nontransaction tx(cx);
    pqxx::result const res(tx.exec(sql));

    // NOLINTNEXTLINE(modernize-loop-convert)
    for (pqxx::result::const_iterator i = res.begin(); i != res.end(); ++i)
    {
      std::cout << "CIK = " << (*i)[0].view() << '\n';
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
