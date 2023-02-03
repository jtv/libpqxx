#include <iostream>
#include <pqxx/pqxx>

int main()
{
  try
  {
    pqxx::connection conn;
    pqxx::transaction tx{conn};
    for (auto const &[id, payload] : tx.stream<long, std::string_view>(
      "SELECT generate_series, 'row #' || generate_series "
      "FROM generate_series(1, 100000000)"))
    {
      std::cout << id << ": " << payload << '\n';
    }
  }
  catch (std::exception const &e)
  {
    std::cerr << "ERROR: " << e.what() << '\n';
    return 1;
  }
}
