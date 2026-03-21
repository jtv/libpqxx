#ifndef PQXX_INTERNAL_GATES_CONNECTION_PIPELINE_HXX
#define PQXX_INTERNAL_GATES_CONNECTION_PIPELINE_HXX

#include <pqxx/internal/callgate.hxx>

#include "pqxx/pipeline.hxx"

namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_pipeline final : callgate<connection>
{
  friend class pqxx::pipeline;

  explicit constexpr connection_pipeline(reference x) noexcept : super(x) {}

  PQXX_ZARGS void start_exec(char const query[]) { home().start_exec(query); }
  [[nodiscard]] pqxx::internal::pq::PGresult *get_result()
  {
    return home().get_result();
  }
  void cancel_query(sl loc) { home().cancel_query(loc); }

  bool consume_input() noexcept { return home().consume_input(); }
  [[nodiscard]] bool is_busy() const noexcept { return home().is_busy(); }

  [[nodiscard]] int encoding_id(sl loc) const
  {
    return home().encoding_id(loc);
  }

  [[nodiscard]] auto get_notice_waiters() const
  {
    return home().m_notice_waiters;
  }
};
} // namespace pqxx::internal::gate
#endif
