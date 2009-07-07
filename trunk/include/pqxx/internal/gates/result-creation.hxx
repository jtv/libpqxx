#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE result_creation : callgate<const result>
{
  friend class pqxx::connection_base;
  friend class pqxx::pipeline;

  result_creation(reference x) : super(x) {}

  static result create(
	internal::pq::PGresult *rhs,
	int protocol,
	const PGSTD::string &query,
	int encoding_code)
  {
    return result(rhs, protocol, query, encoding_code);
  }

  void CheckStatus() const { return home().CheckStatus(); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
