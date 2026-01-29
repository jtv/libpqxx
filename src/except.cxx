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

// Define virtual destructors here so compilers will output vtables here, once.
//
// When compiling a class with virtual functions, compilers generally emit the
// vtable in the translation unit(s) where they find the definition of its
// first non-pure-virtual, non-inline virtual member function.  Or perhaps the
// destructor.
//
// Either way it's easy to end up with lots of redundant definitions of the
// same vtable across many object files, which is a waste of space and time.
// Therefore, we declare each exception type's destructor as the first virtual
// member function, and put its definition here.
namespace pqxx
{
failure::~failure() noexcept = default;
broken_connection::~broken_connection() noexcept = default;
version_mismatch::~version_mismatch() noexcept = default;
variable_set_to_null::~variable_set_to_null() noexcept = default;
sql_error::~sql_error() noexcept = default;
protocol_violation::~protocol_violation() noexcept = default;
in_doubt_error::~in_doubt_error() noexcept = default;
transaction_rollback::~transaction_rollback() noexcept = default;
serialization_failure::~serialization_failure() noexcept = default;
statement_completion_unknown::~statement_completion_unknown() noexcept =
  default;
deadlock_detected::~deadlock_detected() noexcept = default;
internal_error::~internal_error() noexcept = default;

// Special case: We add a prefix to the message.
internal_error::internal_error(std::string const &whatarg, sl loc, st &&tr) :
        failure{
          std::format("LIBPQXX INTERNAL ERROR: {}", whatarg), loc,
          std::move(tr)}
{}

usage_error::~usage_error() noexcept = default;
argument_error::~argument_error() noexcept = default;
conversion_error::~conversion_error() noexcept = default;
unexpected_null::~unexpected_null() noexcept = default;
conversion_overrun::~conversion_overrun() noexcept = default;
range_error::~range_error() noexcept = default;
unexpected_rows::~unexpected_rows() noexcept = default;
feature_not_supported::~feature_not_supported() noexcept = default;
data_exception::~data_exception() noexcept = default;
integrity_constraint_violation::~integrity_constraint_violation() noexcept =
  default;
restrict_violation::~restrict_violation() noexcept = default;
not_null_violation::~not_null_violation() noexcept = default;
foreign_key_violation::~foreign_key_violation() noexcept = default;
unique_violation::~unique_violation() noexcept = default;
check_violation::~check_violation() noexcept = default;
invalid_cursor_state::~invalid_cursor_state() noexcept = default;
invalid_sql_statement_name::~invalid_sql_statement_name() noexcept = default;
invalid_cursor_name::~invalid_cursor_name() noexcept = default;
syntax_error::~syntax_error() noexcept = default;
undefined_column::~undefined_column() noexcept = default;
undefined_function::~undefined_function() noexcept = default;
undefined_table::~undefined_table() noexcept = default;
insufficient_privilege::~insufficient_privilege() noexcept = default;
insufficient_resources::~insufficient_resources() noexcept = default;
disk_full::~disk_full() noexcept = default;
server_out_of_memory::~server_out_of_memory() noexcept = default;
too_many_connections::~too_many_connections() noexcept = default;
plpgsql_error::~plpgsql_error() noexcept = default;
plpgsql_raise::~plpgsql_raise() noexcept = default;
plpgsql_no_data_found::~plpgsql_no_data_found() noexcept = default;
plpgsql_too_many_rows::~plpgsql_too_many_rows() noexcept = default;
} // namespace pqxx
