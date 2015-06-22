/*-------------------------------------------------------------------------
 *
 *   FILE
 *	nontransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::nontransaction class.
 *   pqxx::nontransaction provides nontransactional database access
 *
 * Copyright (c) 2002-2015, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/nontransaction"


pqxx::nontransaction::~nontransaction()
{
  End();
}


pqxx::result pqxx::nontransaction::do_exec(const char Query[])
{
  return DirectExec(Query, 0);
}

