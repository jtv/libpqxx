#include <pqxx/internal/callgate.hxx>

namespace pqxx
{
class connection;
}

namespace pqxx::internal::gate
{
class PQXX_PRIVATE connection_transaction final : callgate<connection>
{
  friend class pqxx::transaction_base;

  constexpr connection_transaction(reference x) noexcept : super(x) {}

  template<typename STRING>
  result exec(STRING query, std::string_view desc, sl loc)
  {
    return home().exec(query, desc, loc);
  }

  template<typename STRING> result exec(STRING query, sl loc)
  {
    return home().exec(query, "", loc);
  }

  void register_transaction(transaction_base *t)
  {
    home().register_transaction(t);
  }
  void unregister_transaction(transaction_base *t) noexcept
  {
    home().unregister_transaction(t);
  }

  auto read_copy_line(sl loc) { return home().read_copy_line(loc); }
  void write_copy_line(std::string_view line, sl loc)
  {
    home().write_copy_line(line, loc);
  }
  void end_copy_write(sl loc) { home().end_copy_write(loc); }

  result exec_prepared(
    std::string_view statement, internal::c_params const &args, sl loc)
  {
    return home().exec_prepared(statement, args, loc);
  }

  result
  exec_params(std::string_view query, internal::c_params const &args, sl loc)
  {
    return home().exec_params(query, args, loc);
  }
};
} // namespace pqxx::internal::gate
