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
  m_Size(size_unknown)
{
  // We can't accept granularity of 1 here, because some block number 
  // arithmetic might overflow.
  if (m_Granularity <= 1)
    throw out_of_range("Invalid CachedResult granularity");
}


void pqxx::CachedResult::clear()
{
  m_Cache.clear();
}


void pqxx::CachedResult::MoveTo(blocknum Block) const
{
  if (Block < 0)
    throw out_of_range("Negative result set index");

  m_Cursor.MoveTo(FirstRowOf(Block));
}


pqxx::Result pqxx::CachedResult::Fetch() const
{
  Result R;
  size_type Pos = m_Cursor.Pos();

  if (m_Cursor >> R) m_Cache.insert(make_pair(BlockFor(Pos), CacheEntry(R)));

  if ((m_Size == size_unknown) && (R.size() < m_Granularity))
  {
    // This is the last block.  While we're here, record result set size.
    m_Size = m_Cursor.Pos();
  }

  return R;
}


void pqxx::CachedResult::DetermineSize() const
{
  m_Cursor.Move(Cursor::ALL());
  m_Size = -m_Cursor.Move(Cursor::BACKWARD_ALL()) - 1;
}

