#ifndef PQXX_H_RESULT_DATA
#define PQXX_H_RESULT_DATA

#include <string>

#include "pqxx/internal/libpq-forward.hxx"


namespace pqxx
{
namespace internal
{

/// Information shared between all copies of a result set
struct PQXX_PRIVATE result_data
{
  /// Underlying libpq-managed result set
  /** @warning This member is duplicated in the result object as a performance
   * shortcut.
   */
  pqxx::internal::pq::PGresult *data;

  /// Frontend/backend protocol version
  int protocol;

  /// Query string that yielded this result
  std::string query;

  int encoding_code;

  // TODO: Locking for result copy-construction etc. also goes here

  result_data();
  result_data(pqxx::internal::pq::PGresult *,
		int protocol,
		const std::string &,
		int encoding_code);
  ~result_data();
};


void PQXX_LIBEXPORT freemem_result_data(const result_data *) PQXX_NOEXCEPT;

} // namespace pqxx::internal
} // namespace pqxx

#endif
