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

// TODO: Forward-only mode of operation?  Or write separate stream class?

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
 * CAUTION: PostgreSQL currently doesn't always let you move cursors backwards,
 * which is a feature this class relies upon.  As a result, CachedResult will
 * only work on certain types of queries.  To make things worse, there is no
 * documentation to define exactly which queries those are.  Therefore the only
 * way to use CachedResult at this time is to test carefully.  Hopefully this
 * can be fixed in the future.
 */
class PQXX_LIBEXPORT CachedResult
{
public:
  typedef Result::size_type size_type;
  typedef size_type blocknum;
  typedef Result::Tuple Tuple;

  /** Perform query and transparently fetch and cache resulting data.
   * Granularity determines how large the blocks of data used internally will
   * be; must be at least 2.
   */
  explicit CachedResult(pqxx::TransactionItf &,
                        const char Query[],
			const PGSTD::string &BaseName="query",
                        size_type Granularity=100);

  // TODO: Iterators, begin(), end()
  // TODO: Metadata
  // TODO: Block replacement (limit cache size); then add capacity()

  const Tuple operator[](size_type i) const
  {
    return GetBlock(BlockFor(i))[Offset(i)];
  }

  const Tuple at(size_type i) const
  {
    return GetBlock(BlockFor(i)).at(Offset(i));
  }

  /// Number of rows in result set.  
  /** Figuring out the size of the result set for the first time may take a lot
   * of time and network traffic, as the CachedResult's internal cursor scans
   * back and forth in search of the set's last row.  Some 30 blocks of data
   * may be fetched in the process.
   */
  size_type size() const
  {
    if (m_Size == -1) DetermineSize();
    return m_Size;
  }

  
  /// Is the result set empty, i.e. does it contain no rows?  May fetch 1 block.
  bool empty() const
  {
    return (m_Size == 0) || (m_Cache.empty() && GetBlock(0).empty());
  }

  /// Drop all data in internal cache, freeing up memory.
  void clear();

  /// NOT IMPLEMENTED YET
  class const_iterator
  {
    const CachedResult &m_Home;
    CachedResult::size_type m_Row;
  public:
    explicit const_iterator(const CachedResult &Home) : m_Home(Home), m_Row(0){}

  private:
    // Not allowed:
    const_iterator();
  };

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
   * highest block known to exist (in m_Lower) and the lowest block known not 
   * to exist (in m_Upper) to narrow the search range as much as possible.
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

  /** Lower and upper bounds to end-of-data: highest block known to exist, and
   * lowest block known not to exist, respectively.  Once size is known, the
   * two will be adjacent, with m_Lower referring to the last, probably non-full
   * block and m_Upper equal to m_Lower+1.
   */
  mutable blocknum m_Lower, m_Upper;

  // Not allowed:
  CachedResult();
  CachedResult(const CachedResult &);
  CachedResult &operator=(const CachedResult &);
};


} // namespace pqxx

#endif

