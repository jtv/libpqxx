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
#include "pg_cursor.h"
#include "pg_result.h"
#include "pg_transaction.h"

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
  string Cmd = "FETCH ";

  // Don't use ALL; use largest Result_size_type value instead.  In the case
  // where this makes any difference (very large result sets), we wouldn't be
  // able to address the whole set with Result_size_type anyway.
#ifdef DIALECT_POSTGRESQL
  // We're allowed to use PostgreSQL dialect.
  // In PostgreSQL, a count of 0 means ALL, and negative means BACKWARD.  
  Cmd += ToString(Count);
#else
  // Generate standard SQL.
  if (Count < 0) Cmd += "BACKWARD ";
  Cmd += ToString(abs(Count));
#endif

  return Cmd + " IN " + m_Name;
}

