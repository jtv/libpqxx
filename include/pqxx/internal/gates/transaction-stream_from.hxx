#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE transaction_stream_from : callgate<transaction_base>
{
  friend class pqxx::stream_from;

  transaction_stream_from(reference x) : super(x) {}

  void BeginCopyRead(std::string_view table, const std::string &columns)
  {
    home().BeginCopyRead(table, columns);
  }

  bool read_copy_line(std::string &line)
  {
    return home().read_copy_line(line);
  }
};
} // namespace pqxx::internal::gate
