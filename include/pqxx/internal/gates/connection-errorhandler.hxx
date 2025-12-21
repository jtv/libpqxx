#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class connection;
class errorhandler;
} // namespace pqxx

namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_errorhandler final : callgate<connection>
{
  friend class pqxx::errorhandler;

  constexpr connection_errorhandler(reference x) noexcept : super(x) {}

  void register_errorhandler(errorhandler *h)
  {
    home().register_errorhandler(h);
  }
  void unregister_errorhandler(errorhandler *h)
  {
    home().unregister_errorhandler(h);
  }
};
} // namespace pqxx::internal::gate
