/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_cursor.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::Cursor class.
 *   Pg::Cursor represents a database cursor.
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/cursor.h"
#include "pqxx/result.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


Pg::Cursor::Cursor(Pg::Transaction &T, 
		   const char Query[],
		   string BaseName,
		   Pg::Result_size_type Count) :
  m_Trans(T),
  m_Name(BaseName),
  m_Count(Count),
  m_Done(false)
{
  // Give ourselves a locally unique name based on connection name
  m_Name += "_" + T.Name() + "_" + ToString(T.GetUniqueCursorNum());

  m_Trans.Exec(("DECLARE " + m_Name + " CURSOR FOR " + Query).c_str());
}


Pg::Result_size_type Pg::Cursor::SetCount(Pg::Result_size_type Count)
{
  Result_size_type Old = m_Count;
  m_Done = false;
  m_Count = Count;
  return Old;
}


// TODO: Maybe splice stream interface (with m_Done & m_Count) out of Cursor?
Pg::Cursor &Pg::Cursor::operator>>(Pg::Result &R)
{
  R = Fetch(m_Count);
  m_Done = R.empty();
  return *this;
}


Pg::Result Pg::Cursor::Fetch(Pg::Result_size_type Count)
{
  Pg::Result R;

  if (Count == 0) 
  {
    m_Trans.MakeEmpty(R);
    return R;
  }

  R = m_Trans.Exec(MakeFetchCmd(Count).c_str());
  m_Done = false;
  return R;
}


void Pg::Cursor::Move(Pg::Result_size_type Count)
{
  if (Count == 0) return;

  m_Done = false;

#ifdef DIALECT_POSTGRESQL
  m_Trans.Exec(("MOVE " + ToString(Count) + " IN " + m_Name).c_str());
#else
  // Standard SQL doesn't have a MOVE command.  Use a FETCH instead, and ignore
  // its results.
  m_Trans.Exec(MakeFetchCmd(Count).c_str());
#endif
}


string Pg::Cursor::MakeFetchCmd(Pg::Result_size_type Count) const
{
  if (!Count) throw logic_error("Internal libpqxx error: Cursor: zero count");
  return "FETCH " + ToString(Count) + " IN " + m_Name;
}

