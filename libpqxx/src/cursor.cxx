/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cursor.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::Cursor class.
 *   pqxx::Cursor represents a database cursor.
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

#include <cstdlib>

#include "pqxx/cursor.h"
#include "pqxx/result"
#include "pqxx/transaction"

using namespace PGSTD;


void pqxx::Cursor::init(const string &BaseName, const char Query[])
{
  // Give ourselves a locally unique name based on connection name
  m_Name += "\"" + 
            BaseName + "_" + 
	    m_Trans.name() + "_" + 
	    ToString(m_Trans.GetUniqueCursorNum()) +
	    "\"";

  m_Trans.Exec("DECLARE " + m_Name + " SCROLL CURSOR FOR " + Query);
}


pqxx::Cursor::size_type pqxx::Cursor::SetCount(size_type Count)
{
  size_type Old = m_Count;
  m_Done = false;
  m_Count = Count;
  return Old;
}


pqxx::Cursor &pqxx::Cursor::operator>>(pqxx::result &R)
{
  R = Fetch(m_Count);
  m_Done = R.empty();
  return *this;
}


pqxx::result pqxx::Cursor::Fetch(size_type Count)
{
  result R;

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

  NormalizedMove(Count, R.size());

  return R;
}


pqxx::result::size_type pqxx::Cursor::Move(size_type Count)
{
  if (!Count) return 0;
  if ((Count < 0) && (m_Pos == pos_start)) return 0;

  m_Done = false;
  const string Cmd( "MOVE " + OffsetString(Count) + " IN " + m_Name);
  long int A = 0;

  try
  {
    result R( m_Trans.Exec(Cmd.c_str()) );
    if (!sscanf(R.CmdStatus(), "MOVE %ld", &A))
      throw runtime_error("Didn't understand database's reply to MOVE: "
	                    "'" + string(R.CmdStatus()) + "'");
  }
  catch (const exception &)
  {
    m_Pos = pos_unknown;
    throw;
  }

  return NormalizedMove(Count, A);
}


pqxx::Cursor::size_type pqxx::Cursor::NormalizedMove(size_type Intended,
                                                     size_type Actual)
{
  if (Actual < 0) 
    throw logic_error("libpqxx internal error: Negative rowcount");
  if (Actual > labs(Intended))
    throw logic_error("libpqxx internal error: Moved/fetched too many rows "
	              "(wanted " + ToString(Intended) + ", "
		      "got " + ToString(Actual) + ")");

  size_type Offset = Actual;

  if (m_Pos == pos_unknown)
  {
    if (Actual < labs(Intended))
    {
      if (Intended < 0)
      {
	// Must have gone back to starting position
	m_Pos = pos_start;
      }
      else if (m_Size == pos_unknown)
      {
	// Oops.  We'd want to set result set size at this point, but we can't
	// because we don't know our position.
	// TODO: Deal with this more elegantly.
	throw runtime_error("Can't determine result set size: "
	                    "Cursor position unknown at end of set");
      }
    }
    // Nothing more we can do to update our position
    return (Intended > 0) ? Actual : -Actual;
  }

  if (Actual < labs(Intended))
  {
    // There is a nonexistant row before the first one in the result set, and 
    // one after the last row, where we may be positioned.  Unfortunately 
    // PostgreSQL only reports "real" rows, making it really hard to figure out
    // how many rows we've really moved.
    if (Actual)
    {
      // We've moved off either edge of our result set; add the one, 
      // nonexistant row that wasn't counted in the status string we got.
      Offset++;
    }
    else if (Intended < 0)
    {
      // We've either moved off the "left" edge of our result set from the 
      // first actual row, or we were on the nonexistant row before the first
      // actual row and so didn't move at all.  Just set up Actual so that we
      // end up at our starting position, which is where we must be.
      Offset = m_Pos - pos_start;
    }
    else if (m_Size != pos_unknown)
    {
      // We either just walked off the right edge (moving at least one row in 
      // the process), or had done so already (in which case we haven't moved).
      // In the case at hand, we already know where the right-hand edge of the
      // result set is, so we use that to compute our offset.
      Offset = (m_Size + pos_start + 1) - m_Pos;
    }
    else
    {
      // This is the hard one.  Assume that we haven't seen the "right edge"
      // before, because m_Size hasn't been set yet.  Therefore, we must have 
      // just stepped off the edge (and m_Size will be set now).
      Offset++;
    }

    if ((Offset > labs(Intended)) && (m_Pos != pos_unknown))
    {
      m_Pos = pos_unknown;
      throw logic_error("libpqxx internal error: Confused cursor position");
    }
  }

  if (Intended < 0) Offset = -Offset;
  m_Pos += Offset;

  if ((Intended > 0) && (Actual < Intended) && (m_Size == pos_unknown))
    m_Size = m_Pos - pos_start - 1;

  m_Done = !Actual;

  return Offset;
}


void pqxx::Cursor::MoveTo(size_type Dest)
{
  // If we don't know where we are, go back to the beginning first.
  if (m_Pos == pos_unknown) Move(BACKWARD_ALL());

  Move(Dest - Pos());
}


pqxx::Cursor::size_type pqxx::Cursor::ALL() throw ()
{
  return numeric_limits<result::size_type>::max();
}


pqxx::Cursor::size_type pqxx::Cursor::BACKWARD_ALL() throw ()
{
  return numeric_limits<result::size_type>::min() + 1;
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


