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

namespace pqxx
{
failure::failure(sl loc) : m_block{std::make_shared<block>(std::move(loc))} {}

failure::failure(std::string whatarg, sl loc) :
        m_block{std::make_shared<block>(std::move(whatarg), std::move(loc))}
{}

failure::failure(
  std::string whatarg, std::string stat, std::string sqls, sl loc) :
        m_block{std::make_shared<block>(
          std::move(whatarg), std::move(stat), std::move(sqls),
          std::move(loc))}
{}

failure::~failure() noexcept = default;

char const *failure::what() const noexcept
{
  return m_block->message.c_str();
}

} // namespace pqxx


pqxx::broken_connection::broken_connection(sl loc) :
        failure{"Connection to database failed.", loc}
{}


pqxx::broken_connection::broken_connection(
  std::string const &whatarg, sl loc) :
        failure{whatarg, loc}
{}


pqxx::protocol_violation::protocol_violation(
  std::string const &whatarg, sl loc) :
        broken_connection{whatarg, loc}
{}


pqxx::variable_set_to_null::variable_set_to_null(
  std::string const &whatarg, sl loc) :
        failure{whatarg, loc}
{}


pqxx::sql_error::sql_error(
  std::string const &whatarg, std::string const &stmt, std::string const &sqls,
  sl loc) :
        failure{whatarg, stmt, sqls, loc}
{}


pqxx::sql_error::~sql_error() = default;


pqxx::in_doubt_error::in_doubt_error(std::string const &whatarg, sl loc) :
        failure{whatarg, loc}
{}


pqxx::transaction_rollback::transaction_rollback(
  std::string const &whatarg, std::string const &q,
  std::string const &sqlstate, sl loc) :
        sql_error{whatarg, q, sqlstate, loc}
{}


pqxx::serialization_failure::serialization_failure(
  std::string const &whatarg, std::string const &q,
  std::string const &sqlstate, sl loc) :
        transaction_rollback{whatarg, q, sqlstate, loc}
{}


pqxx::statement_completion_unknown::statement_completion_unknown(
  std::string const &whatarg, std::string const &q,
  std::string const &sqlstate, sl loc) :
        transaction_rollback{whatarg, q, sqlstate, loc}
{}


pqxx::deadlock_detected::deadlock_detected(
  std::string const &whatarg, std::string const &q,
  std::string const &sqlstate, sl loc) :
        transaction_rollback{whatarg, q, sqlstate, loc}
{}


pqxx::internal_error::internal_error(std::string const &whatarg, sl loc) :
        std::logic_error{std::format("libpqxx internal error: {}", whatarg)},
        location{loc}
{}


pqxx::usage_error::usage_error(std::string const &whatarg, sl loc) :
        std::logic_error{whatarg}, location{loc}
{}


pqxx::argument_error::argument_error(std::string const &whatarg, sl loc) :
        invalid_argument{whatarg}, location{loc}
{}


pqxx::conversion_error::conversion_error(std::string const &whatarg, sl loc) :
        domain_error{whatarg}, location{loc}
{}


pqxx::unexpected_null::unexpected_null(std::string const &whatarg, sl loc) :
        conversion_error{whatarg, loc}
{}


pqxx::conversion_overrun::conversion_overrun(
  std::string const &whatarg, sl loc) :
        conversion_error{whatarg, loc}
{}


pqxx::range_error::range_error(std::string const &whatarg, sl loc) :
        out_of_range{whatarg}, location{loc}
{}
