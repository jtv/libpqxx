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
#include "pqxx/tablestream.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::tablestream::tablestream(transaction_base &STrans, 
                               const string &SName, 
			       const string &Null) :
  m_Trans(STrans),
  m_Name(SName),
  m_Null(Null)
{
  STrans.RegisterStream(this);
}


pqxx::tablestream::~tablestream()
{
  m_Trans.UnregisterStream(this);
  m_Trans.EndCopy();
}

