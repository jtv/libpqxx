#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
namespace internal
{
namespace gate
{
class PQXX_PRIVATE connection_prepare_declaration : callgate<connection_base>
{
  friend class prepare::declaration;

  connection_prepare_declaration(reference x) : super(x) {}

  void prepare_param_declare(
	const PGSTD::string &statement,
	const PGSTD::string &sqltype,
	prepare::param_treatment treatment)
	{ home().prepare_param_declare(statement, sqltype, treatment); }

  void prepare_param_declare_varargs(
	const PGSTD::string &statement, prepare::param_treatment treatment)
	{ home().prepare_param_declare_varargs(statement, treatment); }
};
} // namespace pqxx::internal::gate
} // namespace pqxx::internal
} // namespace pqxx
