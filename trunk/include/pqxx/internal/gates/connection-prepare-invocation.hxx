#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace prepare
{
class invocation;
} // namespace pqxx::prepare

namespace internal
{
namespace gate
{
class PQXX_PRIVATE connection_prepare_invocation : callgate<connection_base>
{
  friend class pqxx::prepare::invocation;

  connection_prepare_invocation(reference x) : super(x) {}

  result prepared_exec(
	const PGSTD::string &statement,
	const char *const params[],
	const int paramlengths[],
	const int binary[],
	int nparams)
  {
    return home().prepared_exec(
	statement,
	params,
	paramlengths,
	binary,
	nparams);
  }

  bool prepared_exists(const PGSTD::string &statement) const
	{ return home().prepared_exists(statement); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
