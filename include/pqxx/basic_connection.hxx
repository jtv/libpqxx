/** Definition of the pqxx::basic_connection class template.
 *
 * Instantiations of basic_connection bring connections and policies together.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/basic_connection instead.
 *
 * Copyright (c) 2006-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_BASIC_CONNECTION
#define PQXX_H_BASIC_CONNECTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <cstddef>
#include <memory>
#include <string>

#include "pqxx/connection_base.hxx"


namespace pqxx
{

/// Base-class template for all libpqxx connection types.
/** Combines connection_base (the highly complex class implementing essentially
 * all connection-related functionality) with a connection policy (a simpler
 * helper class determining the rules that govern the process of setting up the
 * underlying connection to the backend).
 *
 * The pattern used to combine these classes is the same as for
 * basic_transaction.  Through use of the template mechanism, the policy object
 * is embedded in the basic_connection object so that it does not need to be
 * allocated separately.  This also avoids the need for virtual functions in
 * this class.
 */
template<typename CONNECTPOLICY> class basic_connection :
  public connection_base
{
public:
  basic_connection() :
    connection_base(m_policy),
    m_options(std::string()),
    m_policy(m_options)
	{ init(); }

  /// The parsing of options is the same as libpq's PQconnect.
  /// See: https://www.postgresql.org/docs/10/static/libpq-connect.html
  explicit basic_connection(const std::string &opt) :
    connection_base(m_policy),
    m_options(opt),
    m_policy(m_options)
	{init();}

  /// See: @c basic_connection(const std::string &opt)
  explicit basic_connection(const char opt[]) :
    basic_connection(opt ? std::string(opt) : std::string()) {}

  explicit basic_connection(std::nullptr_t) : basic_connection() {}

  ~basic_connection() noexcept
	{ close(); }

  const std::string &options() const noexcept				//[t01]
	{return m_policy.options();}

private:
  /// Connect string.  @warn Must be initialized before the connector!
  std::string m_options;
  /// Connection policy.  @warn Must be initialized after the connect string!
  CONNECTPOLICY m_policy;
};

} // namespace

#include "pqxx/compiler-internal-post.hxx"

#endif
