/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablestream.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::tablestream class.
 *   pqxx::tablestream provides optimized batch access to a database table
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include "pqxx/tablestream"
#include "pqxx/transaction"

using namespace PGSTD;


pqxx::tablestream::tablestream(transaction_base &STrans, 
                               const string &SName, 
			       const string &Null) :
  m_Trans(STrans),
  m_Name(SName),
  m_Null(Null),
  m_Finished(false)
{
}


pqxx::tablestream::~tablestream() throw ()
{
}


void pqxx::tablestream::register_me()
{
  m_Trans.RegisterStream(this);
}


void pqxx::tablestream::base_close()
{
  m_Finished = true;
  m_Trans.UnregisterStream(this);
}


void pqxx::tablestream::RegisterPendingError(const string &Err) throw ()
{
  m_Trans.RegisterPendingError(Err);
}


