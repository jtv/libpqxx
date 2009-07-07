#include "pqxx/libpq-forward.hxx"

namespace pqxx
{
namespace internal
{
class PQXX_PRIVATE connection_pipeline_gate
{
  friend class pqxx::pipeline;

  connection_pipeline_gate(connection_base &home) : m_home(home) {}

  void start_exec(const PGSTD::string &query) { m_home.start_exec(query); }
  pqxx::internal::pq::PGresult *get_result() { return m_home.get_result(); }
  void cancel_query() { m_home.cancel_query(); }

  bool consume_input() throw () { return m_home.consume_input(); }
  bool is_busy() const throw () { return m_home.is_busy(); }

  int encoding_code() throw () { return m_home.encoding_code(); }

  connection_base &m_home;
};
} // namespace pqxx::internal
} // namespace pqxx
