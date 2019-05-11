#include <pqxx/internal/callgate.hxx>

namespace pqxx::internal
{
class row;
}

namespace pqxx::internal::gate
{
class PQXX_PRIVATE result_row : callgate<result>
{
  friend class pqxx::row;

  result_row(reference x) : super(x) {}

  operator bool()
	{ return bool(home()); }
};
} // namespace pqxx::internal::gate
