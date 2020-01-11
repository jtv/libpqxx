#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_stream_from : callgate<connection>
{
  friend class pqxx::stream_from;

  connection_stream_from(reference x) : super{x} {}

  bool read_copy_line(std::string &line)
  {
    return home().read_copy_line(line);
  }
};
} // namespace pqxx::internal::gate
