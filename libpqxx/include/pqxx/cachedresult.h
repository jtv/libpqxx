/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cachedresult.h
 *
 *   DESCRIPTION
 *      definitions for the pqxx::CachedResult class and support classes.
 *   pqxx::CachedResult is a lazy-fetching, transparently-cached result set
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_CACHEDRESULT_H
#define PQXX_CACHEDRESULT_H

#include <map>

#include "pqxx/cursor.h"
#include "pqxx/result.h"

namespace pqxx
{

class TransactionItf;


/** Cached result set.  Chunks of result data are transparently fetched 
 * on-demand and stored in an internal cache for reuse.  Functionality is 
 * similar to that of Result, with certain restrictions and different 
 * performance characteristics.  A CachedResult must live in the context of a
 * transaction, so that it can fetch further rows as they are needed.
 *
 * The class uses a Cursor internally to fetch results.  Data are not fetched
 * row-by-row, but in chunks of configurable size.  For internal computational
 * reasons, these chunks (called "blocks" here) must be at least 2 rows large.  
 *
 * The maximum size of a query result accessed through a CachedResult is one
 * less than the maximum size of a reqular PostgreSQL query result.
 */
class PQXX_LIBEXPORT CachedResult
{
public:
  typedef Result::size_type size_type;
  typedef size_type blocknum;
  typedef Result::Tuple Tuple;

  /// Granularity must be at least 2.
  explicit CachedResult(pqxx::TransactionItf &,
                        const char Query[],
			PGSTD::string BaseName="query",
                        size_type Granularity=100);

  // TODO: empty(), capacity()
  // TODO: Iterators, begin(), end()
  // TODO: Metadata
  // TODO: Block replacement

  const Tuple operator[](size_type i) const
  {
    return GetBlock(BlockFor(i))[Offset(i)];
  }

  const Tuple at(size_type i) const
  {
    return GetBlock(BlockFor(i)).at(Offset(i));
  }

  size_type size() const
  {
    if (m_Size == -1) DetermineSize();
    return m_Size;
  }

  void clear();

private:

  class CacheEntry
  {
    int m_RefCount;
    Result m_Data;

  public:
    CacheEntry() : m_RefCount(0), m_Data() {}
    explicit CacheEntry(const Result &R) : m_RefCount(0), m_Data(R) {}

    const Result &Data() const { return m_Data; }
    int RefCount() const { return m_RefCount; }
  };


  blocknum BlockFor(size_type Row) const { return Row / m_Granularity; }
  size_type Offset(size_type Row) const { return Row % m_Granularity; }

  void MoveTo(blocknum) const;

  /// Get block we're currently at.  Assumes it wasn't in cache already.
  Result Fetch() const;

  Result GetBlock(blocknum b) const
  {
    CacheMap::const_iterator i = m_Cache.find(b);
    if (i != m_Cache.end()) return i->second.Data();

    MoveTo(b);
    return Fetch();
  }

  /** Figure out how big our result set is.  This may take some scanning back
   * and forth, since there's no direct way to find out.  We keep track of the
   * highest block known to exist (which is at the top of our cache) and the
   * lowest block known not to exist (in m_Upper) to narrow the search range as
   * much as possible.
   */
  void DetermineSize() const;

  /// Block size.
  size_type m_Granularity;

  typedef PGSTD::map<blocknum, CacheEntry> CacheMap;
  mutable CacheMap m_Cache;

  mutable Cursor m_Cursor;

  /// Current cursor position (in blocks), or -1 if unknown.
  mutable blocknum m_Pos;

  /// Result set size in rows, or -1 if unknown.
  mutable size_type m_Size;

  /// Upper bound: lowest block known not to exist.
  mutable blocknum m_Upper;

  // Not allowed:
  CachedResult(const CachedResult &);
  CachedResult &operator=(const CachedResult &);
};


} // namespace pqxx

#endif

