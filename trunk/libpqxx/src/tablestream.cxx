/*-------------------------------------------------------------------------
 *
 *   FILE
 *	tablestream.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::TableStream class.
 *   pqxx::TableStream provides optimized batch access to a database table
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/tablestream.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::TableStream::TableStream(Transaction_base &STrans, 
                               const string &SName, 
			       const string &Null) :
  m_Trans(STrans),
  m_Name(SName),
  m_Null(Null)
{
  STrans.RegisterStream(this);
}


pqxx::TableStream::~TableStream()
{
  m_Trans.UnregisterStream(this);
  m_Trans.EndCopy();
}

