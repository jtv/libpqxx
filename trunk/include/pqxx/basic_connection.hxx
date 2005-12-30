/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/basic_connection.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::basic_connection class template
 *   Instantiations of basic_connection bring connections and policies together
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection_base instead.
 *
 * Copyright (c) 2005, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"

#include <string>

#include "pqxx/connection_base"

namespace pqxx
{

// TODO: Also mix in thread synchronization policy here!
template<typename CONNECTPOLICY> class basic_connection : 
  public connection_base
{
public:
  basic_connection() : 
    connection_base(m_policy),
    m_options(PGSTD::string()),
    m_policy(m_options)
	{ init(); }

  explicit basic_connection(const PGSTD::string &opt) :
    connection_base(m_policy), m_options(opt), m_policy(m_options) {init();}

  explicit basic_connection(const char opt[]) :
    connection_base(m_policy),
    m_options(opt?opt:PGSTD::string()),
    m_policy(m_options)
	{ init(); }

  ~basic_connection() throw () { close(); }

  const PGSTD::string &options() const throw ()				//[t1]
	{return m_policy.options();}

private:
  /// Connect string.  @warn Must be initialized before the connector!
  PGSTD::string m_options;
  /// Connection policy.  @warn Must be initialized after the connect string!
  CONNECTPOLICY m_policy;
};

} // namespace

