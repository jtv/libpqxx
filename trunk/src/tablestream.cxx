/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablestream.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablestream class.
 *   pqxx::tablestream provides optimized batch access to a database table
 *
 * Copyright (c) 2001-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/tablestream"
#include "pqxx/transaction"

using namespace PGSTD;


pqxx::tablestream::tablestream(transaction_base &STrans,
	const PGSTD::string &Null) :
  internal::namedclass("tablestream"),
  internal::transactionfocus(STrans),
  m_Null(Null),
  m_Finished(false)
{
}


pqxx::tablestream::~tablestream() throw ()
{
}


void pqxx::tablestream::base_close()
{
  if (!is_finished())
  {
    m_Finished = true;
    unregister_me();
  }
}



