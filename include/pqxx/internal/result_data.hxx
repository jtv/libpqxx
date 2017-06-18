#ifndef PQXX_H_RESULT_DATA
#define PQXX_H_RESULT_DATA

#include <string>

#include "pqxx/internal/libpq-forward.hxx"


namespace pqxx
{
namespace internal
{

/// Information shared between all copies of a result set.
/** Holds the actual libpq result data, and deletes it on destruction.
 * A pqxx::result acts as a reference-counting smart pointer to a
 * result_data.
 */
struct PQXX_PRIVATE result_data
{
  /// Underlying libpq-managed result set
  /** @warning This member is duplicated in the result object as a performance
   * shortcut.
   */
  pqxx::internal::pq::PGresult *data;

  /// Query string that yielded this result
  std::string query;

  result_data();
  result_data(pqxx::internal::pq::PGresult *, const std::string &);
  ~result_data();
};


void PQXX_LIBEXPORT freemem_result_data(const result_data *) PQXX_NOEXCEPT;

} // namespace pqxx::internal
} // namespace pqxx

#endif
