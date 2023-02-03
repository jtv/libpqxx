#if !defined(PQXX_H_CONNECTION_STREAM_FROM)
#define PQXX_H_CONNECTION_STREAM_FROM

#include <pqxx/internal/callgate.hxx>

#include "pqxx/connection.hxx"

namespace pqxx::internal::gate
{
// Not publicising this call gate to specific classes.  We also use it in
// stream_query, which is a template.
struct PQXX_PRIVATE connection_stream_from : callgate<connection>
{
  connection_stream_from(reference x) : super{x} {}

  auto read_copy_line() { return home().read_copy_line(); }
  auto next_copy_line(char *buffer, std::size_t capacity) { return home().next_copy_line(buffer, capacity); }
};
} // namespace pqxx::internal::gate
#endif
