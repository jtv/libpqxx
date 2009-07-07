#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class notify_listener;

namespace internal
{
namespace gate
{
class PQXX_PRIVATE connection_notify_listener : callgate<connection_base>
{
  friend class pqxx::notify_listener;

  connection_notify_listener(reference x) : super(x) {}

  void add_listener(notify_listener *listener)
	{ home().add_listener(listener); }
  void remove_listener(notify_listener *listener) throw ()
	{ home().remove_listener(listener); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
