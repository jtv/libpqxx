#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_creation final : callgate<result const>
{
  friend class pqxx::connection;
  friend class pqxx::pipeline;

  constexpr result_creation(reference x) noexcept : super(x) {}

  [[nodiscard]] static result create(
    std::shared_ptr<internal::pq::PGresult> rhs,
    std::shared_ptr<std::string> const &query,
    std::shared_ptr<pqxx::internal::notice_waiters> &notice_waiters,
    encoding_group enc)
  {
    return result(rhs, query, notice_waiters, enc);
  }

  void check_status(std::string_view desc, sl loc) const
  {
    return home().check_status(desc, loc);
  }

  void check_status(sl loc) const { return home().check_status("", loc); }
};
} // namespace pqxx::internal::gate
