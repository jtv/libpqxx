/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/basic_connection.hxx
 *
 *   DESCRIPTION
 *      definition of the pqxx::basic_connection class template
 *   Instantiations of basic_connection bring connections and policies together
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/basic_connection instead.
 *
 * Copyright (c) 2006-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_BASIC_CONNECTION
#define PQXX_H_BASIC_CONNECTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <memory>
#include <string>

#include "pqxx/connection_base"


namespace pqxx
{

// TODO: Also mix in thread synchronization policy here!
/// The ultimate template that defines a connection type
/** Combines connection_base (the highly complex class implementing essentially
 * all connection-related functionality) with a connection policy (a simpler
 * helper class determining the rules that govern the process of setting up the
 * underlying connection to the backend).
 *
 * The pattern used to combine these classes is the same as for
 * basic_transaction.  Through use of the template mechanism, the policy object
 * is embedded in the basic_connection object so that it does not need to be
 * allocated separately.  At the same time this construct avoids the need for
 * any virtual functions in this class, which reduces risks of bugs in
 * construction and destruction; as well as any need to templatize the larger
 * body of code in the connection_base class which might otherwise lead to
 * unacceptable code duplication.
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

  explicit basic_connection(const std::string &opt) :
    connection_base(m_policy), m_options(opt), m_policy(m_options) {init();}

  explicit basic_connection(const char opt[]) :
    connection_base(m_policy),
    m_options(opt?opt:std::string()),
    m_policy(m_options)
	{ init(); }

  ~basic_connection() PQXX_NOEXCEPT				    { close(); }

  const std::string &options() const PQXX_NOEXCEPT			//[t1]
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
