#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE transaction_stream_to : callgate<transaction_base>
{
  friend class pqxx::stream_to;

  transaction_stream_to(reference x) : super(x) {}

  void BeginCopyWrite(
    std::string_view table, const std::string &columns = std::string{})
  {
    home().BeginCopyWrite(table, columns);
  }

  void write_copy_line(std::string_view line) { home().write_copy_line(line); }

  void end_copy_write() { home().end_copy_write(); }
};
} // namespace pqxx::internal::gate
