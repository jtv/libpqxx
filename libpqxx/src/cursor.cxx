/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::Cursor class.
 *   pqxx::Cursor represents a database cursor.
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/cursor.h"
#include "pqxx/result.h"
#include "pqxx/transaction.h"

using namespace PGSTD;


pqxx::Cursor::Cursor(pqxx::TransactionItf &T, 
		     const char Query[],
		     const string &BaseName,
		     size_type Count) :
  m_Trans(T),
  m_Name(BaseName),
  m_Count(Count),
  m_Done(false),
  m_Pos(pos_start)
{
  // Give ourselves a locally unique name based on connection name
  m_Name += "_" + T.Name() + "_" + ToString(T.GetUniqueCursorNum());

  m_Trans.Exec(("DECLARE " + m_Name + " CURSOR FOR " + Query).c_str());
}


pqxx::Cursor::size_type pqxx::Cursor::SetCount(size_type Count)
{
  size_type Old = m_Count;
  m_Done = false;
  m_Count = Count;
  return Old;
}


pqxx::Cursor &pqxx::Cursor::operator>>(pqxx::Result &R)
{
  R = Fetch(m_Count);
  m_Done = R.empty();
  return *this;
}


pqxx::Result pqxx::Cursor::Fetch(size_type Count)
{
  Result R;

  if (!Count)
  {
    m_Trans.MakeEmpty(R);
    return R;
  }

  const string Cmd( MakeFetchCmd(Count) );

  try
  {
    R = m_Trans.Exec(Cmd.c_str());
  }
  catch (const exception &)
  {
    m_Pos = pos_unknown;
    throw;
  }

  const size_type Dist = ((Count > 0) ? R.size() : -R.size());
  m_Done = (R.size() < Count);

  if (-R.size() > Count)           m_Pos = pos_start;
  else if (m_Pos != pos_unknown)   m_Pos += Dist;

  return R;
}


pqxx::Result::size_type pqxx::Cursor::Move(size_type Count)
{
  if (!Count) return 0;
  if ((Count < 0) && (m_Pos == pos_start)) return 0;

  m_Done = false;
  const string Cmd( "MOVE " + OffsetString(Count) + " IN " + m_Name );
  long int A = 0;

  try
  {
    Result R( m_Trans.Exec(Cmd.c_str()) );
    if (!sscanf(R.CmdStatus(), "MOVE %ld", &A))
      throw runtime_error("Didn't understand database's reply to MOVE: "
	                    "'" + string(R.CmdStatus()) + "'");
  }
  catch (const exception &)
  {
    m_Pos = pos_unknown;
    throw;
  }

  // Assumes reported number is never negative.
  if (Count < 0) A = -A;

  // Assumes actual number of rows reported is never greater than requested 
  // number (in absolute values), hence Count < A also implies Count < 0.
  if (Count < A)
  {
    // This is a weird bit of behaviour in Postgres.  MOVE returns the number
    // of rows it would have returned if it were a FETCH, and operations on a
    // cursor increment/decrement their position if necessary before acting on
    // a row.  The upshot of this is that from position n, a MOVE -n will
    // yield the same status string as MOVE -(n-1), i.e. "MOVE [n-1]"...  But
    // the two will not leave the cursor in the same position!  One puts you
    // on the first row, so a FETCH after that will fetch the second row; the
    // other leaves you on the nonexistant row before the first one, so the
    // next FETCH will fetch the first row.
    m_Pos = pos_start;
    A--;	// Compensate for one row not being reported in status string
  }
  else if (m_Pos != pos_unknown) 
  {
    // The regular case
    m_Pos += A;
  }

  return A;
}


void pqxx::Cursor::MoveTo(size_type Dest)
{
  // If we don't know where we are, go back to the beginning first.
  if (m_Pos == pos_unknown) Move(BACKWARD_ALL());

  Move(Dest - Pos());
}


string pqxx::Cursor::OffsetString(size_type Count)
{
  if (Count == ALL()) return "ALL";
  else if (Count == BACKWARD_ALL()) return "BACKWARD ALL";

  return ToString(Count);
}


string pqxx::Cursor::MakeFetchCmd(size_type Count) const
{
  return "FETCH " + OffsetString(Count) + " IN " + m_Name;
}


