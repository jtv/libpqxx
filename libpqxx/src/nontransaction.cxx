/*-------------------------------------------------------------------------
 *
 *   FILE
 *	nontransaction.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::NonTransaction class.
 *   pqxx::Transaction provides nontransactional database access
 *
 * Copyright (c) 2002-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */

#include "pqxx/nontransaction.h"


using namespace PGSTD;

pqxx::NonTransaction::~NonTransaction()
{
  End();
}


pqxx::Result pqxx::NonTransaction::DoExec(const char Query[])
{
  return DirectExec(Query, 2, 0);
}

