#include <pqxx/internal/callgate.hxx>

#include "pqxx/connection.hxx"


namespace pqxx
{
class notification_receiver;
}


namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_notification_receiver final
        : callgate<connection>
{
  friend class pqxx::notification_receiver;

  constexpr connection_notification_receiver(reference x) noexcept : super(x)
  {}

  void add_receiver(notification_receiver *receiver, sl loc)
  {
    home().add_receiver(receiver, loc);
  }
  void remove_receiver(notification_receiver *receiver, sl loc) noexcept
  {
    home().remove_receiver(receiver, loc);
  }
};
} // namespace pqxx::internal::gate
