#include <string>

#include <pqxx/internal/callgate.hxx>
#include <pqxx/internal/libpq-forward.hxx>

namespace pqxx
{
class blob;
class largeobject;
} // namespace pqxx


namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_largeobject final : callgate<connection>
{
  friend class pqxx::blob;
  friend class pqxx::largeobject;

  constexpr connection_largeobject(reference x) noexcept : super(x) {}

  PQXX_PURE [[nodiscard]] pq::PGconn *raw_connection() const noexcept
  {
    return home().raw_connection();
  }
};


class PQXX_PRIVATE const_connection_largeobject final
        : callgate<connection const>
{
  friend class pqxx::blob;
  friend class pqxx::largeobject;

  constexpr const_connection_largeobject(reference x) noexcept : super(x) {}

  std::string error_message() const { return home().err_msg(); }
};
} // namespace pqxx::internal::gate
