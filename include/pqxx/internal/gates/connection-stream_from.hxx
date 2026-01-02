#if !defined(PQXX_H_CONNECTION_STREAM_FROM)
#  define PQXX_H_CONNECTION_STREAM_FROM

#  include <pqxx/internal/callgate.hxx>

namespace pqxx::internal
{
template<typename... TYPE> class stream_query;
} // namespace pqxx::internal


#  include "pqxx/connection.hxx"

namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_stream_from final : callgate<connection>
{
  friend class pqxx::stream_from;
  template<typename... TYPE> friend class pqxx::internal::stream_query;

  constexpr connection_stream_from(reference x) noexcept : super{x} {}

  auto read_copy_line(sl loc) { return home().read_copy_line(loc); }
};
} // namespace pqxx::internal::gate
#endif
