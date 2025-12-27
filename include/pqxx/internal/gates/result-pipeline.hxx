#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_pipeline final : callgate<result const>
{
  friend class pqxx::pipeline;

  constexpr result_pipeline(reference x) noexcept : super(x) {}

  std::shared_ptr<std::string const> query_ptr() const
  {
    return home().query_ptr();
  }
};
} // namespace pqxx::internal::gate
