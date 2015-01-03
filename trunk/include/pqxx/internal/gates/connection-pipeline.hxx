#include <pqxx/internal/callgate.hxx>
#include "pqxx/internal/libpq-forward.hxx"

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE connection_pipeline : callgate<connection_base>
{
  friend class pqxx::pipeline;

  connection_pipeline(reference x) : super(x) {}

  void start_exec(const std::string &query) { home().start_exec(query); }
  pqxx::internal::pq::PGresult *get_result() { return home().get_result(); }
  void cancel_query() { home().cancel_query(); }

  bool consume_input() PQXX_NOEXCEPT { return home().consume_input(); }
  bool is_busy() const PQXX_NOEXCEPT { return home().is_busy(); }

  int encoding_code() { return home().encoding_code(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
