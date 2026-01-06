#include <iostream>
#include <pqxx/pqxx>
#include <stdexcept>

int main(int argc, char *argv[])
{
  try
  {
    if (!argv[1])
      throw std::runtime_error("Give me a string!");

    pqxx::connection cx;
    pqxx::work tx(cx);

    // work::exec() returns a full result set, which can consist of any
    // number of rows.
    pqxx::result r = tx.exec("SELECT $1", pqxx::params{argv[1]});

    // End our transaction here.  We can still use the result afterwards.
    tx.commit();

    // Print the first field of the first row.  Read it as a C string,
    // just like std::string::c_str() does.
    std::cout << r[0][0].c_str() << std::endl;
  }
  catch (std::exception const &e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
