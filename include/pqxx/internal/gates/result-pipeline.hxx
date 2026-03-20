#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_pipeline final : callgate<result const>
{
  friend class pqxx::pipeline;

  explicit constexpr result_pipeline(reference x) noexcept : super(x) {}

  [[nodiscard]] std::shared_ptr<std::string const> query_ptr() const
  {
    return home().query_ptr();
  }
};
} // namespace pqxx::internal::gate
