#include <pqxx/internal/callgate.hxx>

#include "pqxx/stream_to.hxx"


namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_stream_to final : callgate<connection>
{
  friend class pqxx::stream_to;

  constexpr connection_stream_to(reference x) noexcept : super(x) {}

  void write_copy_line(std::string_view line, sl loc)
  {
    home().write_copy_line(line, loc);
  }
  void end_copy_write(sl loc) { home().end_copy_write(loc); }
};
} // namespace pqxx::internal::gate
