/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/cachedresult.h
 *
 *   DESCRIPTION
 *      definitions for the pqxx::cachedresult class and support classes.
 *   pqxx::cachedresult is a lazy-fetching, transparently-cached result set
 *
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
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

/** Cached result set.  Chunks of result data are transparently fetched 
 * on-demand and stored in an internal cache for reuse.  Functionality is 
 * similar to that of result, with certain restrictions and different 
 * performance characteristics.  A cachedresult must live in the context of a
 * backend transaction, so that it can fetch further rows as they are needed.
 * @warning
 * The transaction must have serializable isolation level to ensure that the 
 * result set of the query remains unchanged while parts of it are cached.  This
 * class is to be replaced by a C++-style iterator interface.
 *
 * The class uses a Cursor internally to fetch results.  Data are not fetched
 * row-by-row, but in chunks of configurable size.  For internal computational
 * reasons, these chunks (called "blocks" here) must be at least 2 rows large.  
 *
 * @warning PostgreSQL currently doesn't always let you move cursors backwards,
 * which is a feature this class relies upon.  As a result, cachedresult will
 * only work on certain types of queries.  To make things worse, there is no
 * documentation to define exactly which queries those are.  Therefore the only
 * way to use cachedresult at this time is to test carefully.  Hopefully this
 * can be fixed in the future.
 *
 * @deprecated This class was not ported to the 2.x libpqxx API because it is to
 * be replaced by a container/iterator interface.
 */
class PQXX_LIBEXPORT cachedresult
{
public:
  typedef result::size_type size_type;
  typedef size_type blocknum;

  /// Tuple type.  Currently borrowed from result, but may change in the future.
  typedef result::tuple tuple;

  /// @deprecated For compatibility with old Tuple class
  typedef tuple Tuple;

  /** Perform query and transparently fetch and cache resulting data.
   * @param T is the transaction context in which the cachedresult lives.  This
   * 	will be used whenever data is fetched.  Must have isolation level
   * 	"serializable," otherwise a link error will be generated for the symbol
   * 	error_permitted_isolation_level.
   * @param Query is the SQL query that yields the desired result set.
   * @param BaseName gives the initial part of the name for this cachedresult
   * 	and the Cursor it uses to obtain its results.
   * @param Granularity determines how large the blocks of data used internally
   * will be; must be at least 2.
   */
  template<typename TRANSACTION> explicit
    cachedresult(TRANSACTION &T,
                 const char Query[],
		 const PGSTD::string &BaseName="query",
                 size_type Granularity=100) :				//[t40]
      m_Granularity(Granularity),
      m_Cache(),
      m_Cursor(T, Query, BaseName, Granularity),
      m_EmptyResult(),
      m_HaveEmpty(false)
  {
    // Trigger build error if T has insufficient isolation level
    error_permitted_isolation_level(PQXX_TYPENAME TRANSACTION::isolation_tag());
    init();
  }


  /// Access a tuple.  Invalid index yields undefined behaviour.
  /**
   * Caveat: the tuple contains a reference to a result that may be destroyed
   * by any other operation on the cachedresult, even if its constness is
   * preserved.  Therefore only use the returned tuple as a temporary, and do
   * not try to copy-construct it, or keep references or pointers to it.
   *
   * @param i the number of the tuple to be accessed.
   */
  const tuple operator[](size_type i) const				//[t41]
  	{ return GetBlock(BlockFor(i))[Offset(i)]; }

  /// Access a tuple.  Throws exception if index is out of range.
  /**
   * If the given index is not the index of an existing row, an out_of_range
   * error will be thrown.
   *
   * Caveat: the tuple contains a reference to a result that may be destroyed
   * by any other operation on the cachedresult, even if its constness is
   * preserved.  Therefore only use the returned tuple as a temporary, and do
   * not try to copy-construct it, or keep references or pointers to it.
   *
   * @param i the number of the tuple to be accessed.
   */
   const tuple at(size_type i) const					//[t40]
  	{ return GetBlock(BlockFor(i)).at(Offset(i)); }

  /// Number of rows in result set.  First call may be slow.
  size_type size() const;						//[t40]
  
  /// Is the result set empty, i.e. does it contain no rows?  May fetch 1 block.
  bool empty() const;							//[t47]

private:
  typedef Cursor::pos pos;

#ifndef PQXX_WORKAROUND_VC7
  /// Only defined for permitted isolation levels (in this case, serializable)
  /** If you get a link or compile error saying this function is not defined,
   * that means a cachedresult is being created on a transaction that doesn't
   * have a sufficient isolation level to support the cachedresult's reliable
   * operation.
   */
  template<typename ISOLATIONTAG>
    static inline void error_permitted_isolation_level(ISOLATIONTAG) throw ();

#if defined(__SUNPRO_CC)
  // Incorrect, but needed to compile with Sun CC
  template<> static void 
    error_permitted_level(isolation_traits<serializable>) throw() {}
#endif	// __SUNPRO_CC
#else
  // Incorrect, but needed to compile with Visual C++ 7
  template<> static inline void
    error_permitted_isolation_level(isolation_traits<serializable>) throw ();
#endif

  void init();

  blocknum BlockFor(size_type Row) const throw () 
  	{ return Row / m_Granularity; }
  size_type Offset(size_type Row) const throw ()
  	{ return Row % m_Granularity; }
  Cursor::size_type FirstRowOf(blocknum Block) const throw ()
    	{ return Block*m_Granularity; }

  void MoveTo(blocknum) const;

  /// Get block we're currently at.  Assumes it wasn't in cache already.
  const result &Fetch() const;

  const result &GetBlock(blocknum b) const
  {
    CacheMap::const_iterator i = m_Cache.find(b);
    if (i != m_Cache.end()) return i->second;

    MoveTo(b);
    return Fetch();
  }

  /// Block size.
  size_type m_Granularity;

  typedef PGSTD::map<blocknum, result> CacheMap;
  mutable CacheMap m_Cache;

  mutable Cursor m_Cursor;
  mutable result m_EmptyResult;
  mutable bool   m_HaveEmpty;

  // Not allowed:
  cachedresult();
  cachedresult(const cachedresult &);
  cachedresult &operator=(const cachedresult &);
};

/// @deprecated For compatibility with the old CachedResult class
typedef cachedresult CachedResult;

template<> inline void
cachedresult::error_permitted_isolation_level(isolation_traits<serializable>)
  throw () {}

} // namespace pqxx

#endif

