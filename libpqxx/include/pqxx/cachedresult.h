/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cachedresult.h
 *
 *   DESCRIPTION
 *      definitions for the pqxx::CachedResult class and support classes.
 *   pqxx::CachedResult is a lazy-fetching, transparently-cached result set
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
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

  /// Tuple type.  Currently borrowed from Result, but may change in the future.
  typedef Result::Tuple Tuple;

  /** Perform query and transparently fetch and cache resulting data.
   * @param T is the transaction context in which the CachedResult lives.  This
   * 	will be used whenever data is fetched.
   * @param Query is the SQL query that yields the desired result set.
   * @param BaseName gives the initial part of the name for this CachedResult
   * 	and the Cursor it uses to obtain its results.
   * @param Granularity determines how large the blocks of data used internally
   * will be; must be at least 2.
   */
  explicit CachedResult(pqxx::TransactionItf &T,
                        const char Query[],
			const PGSTD::string &BaseName="query",
                        size_type Granularity=100);			//[t40]

  // TODO: Iterators, begin(), end()
  // TODO: Metadata
  // TODO: Block replacement (limit cache size); then add capacity()

  /// Access a tuple.  Invalid index yields undefined behaviour.
  /**
   * Caveat: the Tuple contains a reference to a Result that may be destroyed
   * by any other operation on the CachedResult, even if its constness is
   * preserved.  Therefore only use the returned Tuple as a temporary, and do
   * not try to copy-construct it, or keep references or pointers to it.
   *
   * @param i the number of the tuple to be accessed.
   */
  const Tuple operator[](size_type i) const				//[t41]
  	{ return GetBlock(BlockFor(i))[Offset(i)]; }

  /// Access a tuple.  Throws exception if index is out of range.
  /**
   * If the given index is not the index of an existing row, an out_of_range
   * error will be thrown.
   *
   * Caveat: the Tuple contains a reference to a Result that may be destroyed
   * by any other operation on the CachedResult, even if its constness is
   * preserved.  Therefore only use the returned Tuple as a temporary, and do
   * not try to copy-construct it, or keep references or pointers to it.
   *
   * @param i the number of the tuple to be accessed.
   */
   const Tuple at(size_type i) const					//[t40]
  	{ return GetBlock(BlockFor(i)).at(Offset(i)); }

  /// Number of rows in result set.  First call may be slow.
  size_type size() const;						//[t40]
  
  /// Is the result set empty, i.e. does it contain no rows?  May fetch 1 block.
  bool empty() const;							//[]

  /// Drop all data in internal cache, freeing up memory.
  void clear();								//[]

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

  typedef Cursor::pos pos;

  blocknum BlockFor(size_type Row) const throw () 
  	{ return Row / m_Granularity; }
  size_type Offset(size_type Row) const throw ()
  	{ return Row % m_Granularity; }
  Cursor::size_type FirstRowOf(blocknum Block) const throw ()
    	{ return Block*m_Granularity; }

  void MoveTo(blocknum) const;

  /// Get block we're currently at.  Assumes it wasn't in cache already.
  const Result &Fetch() const;

  const Result &GetBlock(blocknum b) const
  {
    CacheMap::const_iterator i = m_Cache.find(b);
    if (i != m_Cache.end()) return i->second;

    MoveTo(b);
    return Fetch();
  }

  /// Block size.
  size_type m_Granularity;

  typedef PGSTD::map<blocknum, const Result> CacheMap;
  mutable CacheMap m_Cache;

  mutable Cursor m_Cursor;
  mutable Result m_EmptyResult;
  mutable bool   m_HaveEmpty;

  // Not allowed:
  CachedResult();
  CachedResult(const CachedResult &);
  CachedResult &operator=(const CachedResult &);
};


} // namespace pqxx

#endif

