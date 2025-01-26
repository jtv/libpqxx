/** Implementation of libpqxx exception classes.
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include "pqxx/internal/header-pre.hxx"

#include "pqxx/except.hxx"
#include "pqxx/internal/concat.hxx"

#include "pqxx/internal/header-post.hxx"

pqxx::failure::failure(std::string const &whatarg, std::source_location loc) :
        std::runtime_error{whatarg}, location{loc}
{}


pqxx::broken_connection::broken_connection() :
        failure{"Connection to database failed."}
{}


pqxx::broken_connection::broken_connection(
  std::string const &whatarg, std::source_location loc) :
        failure{whatarg, loc}
{}


pqxx::protocol_violation::protocol_violation(
  std::string const &whatarg, std::source_location loc) :
        broken_connection{whatarg, loc}
{}


pqxx::variable_set_to_null::variable_set_to_null(
  std::string const &whatarg, std::source_location loc) :
        failure{whatarg, loc}
{}


pqxx::sql_error::sql_error(
  std::string const &whatarg, std::string Q, char const *sqlstate,
  std::source_location loc) :
        failure{whatarg, loc},
        m_query{std::move(Q)},
        m_sqlstate{sqlstate ? sqlstate : ""}
{}


pqxx::sql_error::~sql_error() noexcept = default;


PQXX_PURE std::string const &pqxx::sql_error::query() const noexcept
{
  return m_query;
}


PQXX_PURE std::string const &pqxx::sql_error::sqlstate() const noexcept
{
  return m_sqlstate;
}


pqxx::in_doubt_error::in_doubt_error(
  std::string const &whatarg, std::source_location loc) :
        failure{whatarg, loc}
{}


pqxx::transaction_rollback::transaction_rollback(
  std::string const &whatarg, std::string const &q, char const sqlstate[],
  std::source_location loc) :
        sql_error{whatarg, q, sqlstate, loc}
{}


pqxx::serialization_failure::serialization_failure(
  std::string const &whatarg, std::string const &q, char const sqlstate[],
  std::source_location loc) :
        transaction_rollback{whatarg, q, sqlstate, loc}
{}


pqxx::statement_completion_unknown::statement_completion_unknown(
  std::string const &whatarg, std::string const &q, char const sqlstate[],
  std::source_location loc) :
        transaction_rollback{whatarg, q, sqlstate, loc}
{}


pqxx::deadlock_detected::deadlock_detected(
  std::string const &whatarg, std::string const &q, char const sqlstate[],
  std::source_location loc) :
        transaction_rollback{whatarg, q, sqlstate, loc}
{}


pqxx::internal_error::internal_error(std::string const &whatarg) :
        std::logic_error{internal::concat("libpqxx internal error: ", whatarg)}
{}


pqxx::usage_error::usage_error(
  std::string const &whatarg, std::source_location loc) :
        std::logic_error{whatarg}, location{loc}
{}


pqxx::argument_error::argument_error(
  std::string const &whatarg, std::source_location loc) :
        invalid_argument{whatarg}, location{loc}
{}


pqxx::conversion_error::conversion_error(
  std::string const &whatarg, std::source_location loc) :
        domain_error{whatarg}, location{loc}
{}


pqxx::unexpected_null::unexpected_null(
  std::string const &whatarg, std::source_location loc) :
        conversion_error{whatarg, loc}
{}


pqxx::conversion_overrun::conversion_overrun(
  std::string const &whatarg, std::source_location loc) :
        conversion_error{whatarg, loc}
{}


pqxx::range_error::range_error(
  std::string const &whatarg, std::source_location loc) :
        out_of_range{whatarg}, location{loc}
{}
