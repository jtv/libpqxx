/** Implementation of libpqxx exception classes.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
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

#if defined(PQXX_HAVE_SOURCE_LOCATION)
pqxx::failure::failure(std::string const &whatarg, std::source_location loc) :
        std::runtime_error{whatarg}, location{loc}
{}
#else
pqxx::failure::failure(std::string const &whatarg) :
        std::runtime_error{whatarg}
{}
#endif


pqxx::broken_connection::broken_connection() :
        failure{"Connection to database failed."}
{}


pqxx::broken_connection::broken_connection(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        failure{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::protocol_violation::protocol_violation(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        broken_connection{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::variable_set_to_null::variable_set_to_null(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        failure{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::sql_error::sql_error(
  std::string const &whatarg, std::string Q, char const *sqlstate
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        failure{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        },
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
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        failure{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::transaction_rollback::transaction_rollback(
  std::string const &whatarg, std::string const &q, char const sqlstate[]
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        sql_error{
          whatarg, q, sqlstate
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::serialization_failure::serialization_failure(
  std::string const &whatarg, std::string const &q, char const sqlstate[]
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        transaction_rollback{
          whatarg, q, sqlstate
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::statement_completion_unknown::statement_completion_unknown(
  std::string const &whatarg, std::string const &q, char const sqlstate[]
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        transaction_rollback{
          whatarg, q, sqlstate
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::deadlock_detected::deadlock_detected(
  std::string const &whatarg, std::string const &q, char const sqlstate[]
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        transaction_rollback{
          whatarg, q, sqlstate
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::internal_error::internal_error(std::string const &whatarg) :
        std::logic_error{internal::concat("libpqxx internal error: ", whatarg)}
{}


pqxx::usage_error::usage_error(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        std::logic_error{whatarg}
#if defined(PQXX_HAVE_SOURCE_LOCATION)
        ,
        location{loc}
#endif
{}


pqxx::argument_error::argument_error(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        invalid_argument{whatarg}
#if defined(PQXX_HAVE_SOURCE_LOCATION)
        ,
        location{loc}
#endif
{}


pqxx::conversion_error::conversion_error(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        domain_error{whatarg}
#if defined(PQXX_HAVE_SOURCE_LOCATION)
        ,
        location{loc}
#endif
{}


pqxx::unexpected_null::unexpected_null(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        conversion_error{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::conversion_overrun::conversion_overrun(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        conversion_error{
          whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
          ,
          loc
#endif
        }
{}


pqxx::range_error::range_error(
  std::string const &whatarg
#if defined(PQXX_HAVE_SOURCE_LOCATION)
  ,
  std::source_location loc
#endif
  ) :
        out_of_range{whatarg}
#if defined(PQXX_HAVE_SOURCE_LOCATION)
        ,
        location{loc}
#endif
{}
