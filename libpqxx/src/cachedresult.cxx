/*-------------------------------------------------------------------------
 *
 *   FILE
 *	cachedresult.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::CachedResult class.
 *   pqxx::CachedResult transparently fetches and caches query results on demand
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pqxx/cachedresult.h"


using namespace PGSTD;

pqxx::CachedResult::CachedResult(pqxx::TransactionItf &Trans,
                                 const char Query[],
				 const PGSTD::string &BaseName,
				 size_type Granularity) :
  m_Granularity(Granularity),
  m_Cache(),
  m_Cursor(Trans, Query, BaseName, Granularity),
  m_Pos(0), 
  m_Size(-1),
  m_Lower(-1),
  m_Upper(-1)
{
  // We can't accept granularity of 1 here, because some block number 
  // arithmetic might overflow.
  if (m_Granularity <= 1)
    throw out_of_range("Invalid CachedResult granularity");

  if (m_Lower > 0)
    throw logic_error("Internal libpqxx error: "
	              "CachedResult::size_type is unsigned");

  m_Upper = BlockFor(numeric_limits<size_type>::max()) + 1;
}


void pqxx::CachedResult::clear()
{
  m_Cache.clear();
}


void pqxx::CachedResult::MoveTo(blocknum Block) const
{
  if (Block < 0)
    throw out_of_range("Invalid result set index");

  if (Block == m_Pos) return;
  try
  {
    if (m_Pos == -1)
    {
      // We don't know where we are.  Go back to our origin first.
      // Hope this does indeed get us back to our starting point...
      m_Cursor.Move(Cursor::BACKWARD_ALL());
      m_Pos = 0;
    }
    if (Block != m_Pos) m_Cursor.Move((Block - m_Pos) * m_Granularity);
    m_Pos = Block;
  }
  catch (const exception &)
  {
    // After an error, we don't know for sure where our cursor is
    m_Pos = -1;
    throw;
  }
}


pqxx::Result pqxx::CachedResult::Fetch() const
{
  Result R;
  if (m_Pos == -1) 
    throw logic_error("Internal libpqxx error: "
	              "CachedResult fetches data from unknown cursor position");

  if (m_Pos >= m_Upper) 
  {
    m_Pos = -1;
    return R;
  }

  try
  {
    if (!(m_Cursor >> R))
    {
      m_Upper = m_Pos;
      if ((m_Size == -1) && (m_Pos == (m_Lower + 1))) 
      {
        m_Size = m_Pos * m_Granularity;
      }

      m_Pos = -1;
      return R;
    }
    m_Cache.insert(make_pair(m_Pos, CacheEntry(R)));
    if (m_Pos > m_Lower) m_Lower = m_Pos;
    if ((m_Size == -1) && (R.size() < m_Granularity))
    {
      // This is the last block.  While we're here, record result set size.
      m_Upper = m_Pos + 1;
      m_Size = m_Pos * m_Granularity + R.size();
      m_Pos = -1;
    }
    else
    {
      ++m_Pos;
    }
  }
  catch (const exception &)
  {
    // After an error, we don't know for sure where our cursor is
    m_Pos = -1;
    throw;
  }
  return R;
}


void pqxx::CachedResult::DetermineSize() const
{
  if (m_Size != -1) return;

  // Binary search for highest-numbered block we can fetch

  if (m_Cache.empty()) 
  {
    GetBlock(0);
    if (m_Cache.empty())
    {
      m_Size = 0;
      m_Upper = 0;
      return;
    }
  }

  const blocknum TopBlock = BlockFor(numeric_limits<size_type>::max());
  if (m_Upper > TopBlock)
  {
    Result T = GetBlock(TopBlock);
    if (!T.empty())
    {
      if (m_Size <= 0)
	throw runtime_error("Result set too large");
      return;
    }
  }

  while ((m_Lower+1) < m_Upper) GetBlock((m_Lower + m_Upper)/2);

  CacheMap::reverse_iterator L = m_Cache.rbegin();
  m_Size = L->first * m_Granularity + L->second.Data().size();
}

