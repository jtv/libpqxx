/** Implementation of libpqxx exception classes.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <format>

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/except.hxx"

#include "pqxx/internal/header-post.hxx"

// Define virtual functions here so compilers will output vtables here, once.
//
// When compiling a class with virtual functions, compilers generally emit the
// vtable in the translation unit(s) where they find the definition of its
// first non-pure-virtual, non-inline virtual member function.  Or perhaps the
// destructor.
//
// Either way it's easy to end up with lots of redundant definitions of the
// same vtable across many object files, which is a waste of space and time.
// Therefore, we try to define exception classes' first virtual member
// functions here.
namespace pqxx
{
failure::~failure() noexcept = default;
std::string_view failure::name() const noexcept
{
  return "failure";
}
std::string_view broken_connection::name() const noexcept
{
  return "broken_connection";
}
std::string_view version_mismatch::name() const noexcept
{
  return "version_mismatch";
}
std::string_view variable_set_to_null::name() const noexcept
{
  return "variable_set_to_null";
}
std::string_view sql_error::name() const noexcept
{
  return "sql_error";
}
std::string_view protocol_violation::name() const noexcept
{
  return "protocol_violation";
}
std::string_view in_doubt_error::name() const noexcept
{
  return "in_doubt_error";
}
std::string_view transaction_rollback::name() const noexcept
{
  return "transaction_rollback";
}
std::string_view serialization_failure::name() const noexcept
{
  return "serialization_failure";
}
std::string_view statement_completion_unknown::name() const noexcept
{
  return "statement_completion_unknown";
}
std::string_view deadlock_detected::name() const noexcept
{
  return "deadlock_detected";
}
std::string_view internal_error::name() const noexcept
{
  return "internal_error";
}

// Special case: We add a prefix to the message.
internal_error::internal_error(std::string const &whatarg, sl loc, st &&tr) :
        failure{
          std::format("LIBPQXX INTERNAL ERROR: {}", whatarg), loc,
          // When there's no support, clang-tidy complains that our placeholder
          // is trivially copyable and therefore the std::move() is pointless.
          //
          // NOLINTNEXTLINE(performance-move-const-arg)
          std::move(tr)}
{}

std::string_view usage_error::name() const noexcept
{
  return "usage_error";
}
std::string_view argument_error::name() const noexcept
{
  return "argument_error";
}
std::string_view conversion_error::name() const noexcept
{
  return "conversion_error";
}
std::string_view unexpected_null::name() const noexcept
{
  return "unexpected_null";
}
std::string_view conversion_overrun::name() const noexcept
{
  return "conversion_overrun";
}
std::string_view range_error::name() const noexcept
{
  return "range_error";
}
std::string_view unexpected_rows::name() const noexcept
{
  return "unexpected_rows";
}
std::string_view feature_not_supported::name() const noexcept
{
  return "feature_not_supported";
}
std::string_view data_exception::name() const noexcept
{
  return "data_exception";
}
std::string_view integrity_constraint_violation::name() const noexcept
{
  return "integrity_constraint_violation";
}
std::string_view restrict_violation::name() const noexcept
{
  return "restrict_violation";
}
std::string_view not_null_violation::name() const noexcept
{
  return "not_null_violation";
}
std::string_view foreign_key_violation::name() const noexcept
{
  return "foreign_key_violation";
}
std::string_view unique_violation::name() const noexcept
{
  return "unique_violation";
}
std::string_view check_violation::name() const noexcept
{
  return "check_violation";
}
std::string_view invalid_cursor_state::name() const noexcept
{
  return "invalid_cursor_state";
}
std::string_view invalid_sql_statement_name::name() const noexcept
{
  return "invalid_sql_statement_name";
}
std::string_view invalid_cursor_name::name() const noexcept
{
  return "invalid_cursor_name";
}
std::string_view syntax_error::name() const noexcept
{
  return "syntax_error";
}
std::string_view undefined_column::name() const noexcept
{
  return "undefined_column";
}
std::string_view undefined_function::name() const noexcept
{
  return "undefined_function";
}
std::string_view undefined_table::name() const noexcept
{
  return "undefined_table";
}
std::string_view insufficient_privilege::name() const noexcept
{
  return "insufficient_privilege";
}
std::string_view insufficient_resources::name() const noexcept
{
  return "insufficient_resources";
}
std::string_view disk_full::name() const noexcept
{
  return "disk_full";
}
std::string_view server_out_of_memory::name() const noexcept
{
  return "server_out_of_memory";
}
std::string_view too_many_connections::name() const noexcept
{
  return "too_many_connections";
}
std::string_view plpgsql_error::name() const noexcept
{
  return "plpgsql_error";
}
std::string_view plpgsql_raise::name() const noexcept
{
  return "plpgsql_raise";
}
std::string_view plpgsql_no_data_found::name() const noexcept
{
  return "plpgsql_no_data_found";
}
std::string_view plpgsql_too_many_rows::name() const noexcept
{
  return "plpgsql_too_many_rows";
}
} // namespace pqxx
