/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/binarystring.hxx
 *
 *   DESCRIPTION
 *      declarations for bytea (binary string) conversions
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/binarystring instead.
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/libcompiler.h"

#include <string>

#include "pqxx/result"


namespace pqxx
{

/// Reveals "unescaped" version of PostgreSQL bytea string
/** This class represents a postgres-internal buffer containing the original,
 * binary string represented by a field of type bytea.  The raw value returned
 * by such a field contains escape sequences for certain characters, which are
 * filtered out by binarystring.  
 *
 * The resulting string is zero-terminated, but may also contain zero bytes (or
 * indeed any other byte value) so don't assume that it can be treated as a
 * C-style string unless you've made sure of this yourself.
 *
 * The binarystring retains its value even if the result it was obtained from is
 * destroyed, but it cannot be copied or assigned.
 */
class PQXX_LIBEXPORT binarystring : PQAlloc<unsigned char>
{
  // TODO: Templatize on character type?
public:
  typedef unsigned char char_type;
  typedef PGSTD::char_traits<char_type>::char_type value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;
  typedef const value_type &const_reference;
  typedef const value_type *const_pointer;
  typedef const_pointer const_iterator;

#ifndef PQXX_WORKAROUND_VC7
  typedef PGSTD::reverse_iterator<const_iterator> const_reverse_iterator;
#endif

private:
  typedef PQAlloc<value_type> super;

public:
  /// Read and unescape bytea field
  /** The field will be zero-terminated, even if the original bytea field isn't.
   * @param F the field to read; must be a bytea field
   */
  explicit binarystring(const result::field &F); 			//[t62]

  /// Size of converted string in bytes
  size_type size() const throw () { return m_size; }			//[t62]
  size_type length() const throw () { return size(); }			//[t62]
  bool empty() const throw () { return size()==0; }			//[t62]

  const_iterator begin() const throw () { return data(); }		//[t62]
  const_iterator end() const throw () { return data()+m_size; }		//[t62]

#ifndef PQXX_WORKAROUND_VC7
  const_reverse_iterator rbegin() const 				//[t62]
  	{ return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const 					//[t62]
  	{ return const_reverse_iterator(begin()); }
#endif

  /// Unescaped field contents 
  const value_type *data() const throw () {return super::c_ptr();}	//[t62]

  const_reference operator[](size_type i) const throw () 		//[t62]
  	{ return data()[i]; }

  const_reference at(size_type n) const;				//[t62]

  /// Raw character buffer (no terminating zero is added)
  /** @warning No terminating zero is added!  If the binary data did not end in
   * a null character, you will not find one here.
   */
  const char *c_ptr() const throw () 					//[t62]
  {
    return reinterpret_cast<char *>(super::c_ptr());
  }

  /// Read as regular C++ string (may include null characters)
  /** Caches string buffer to speed up repeated reads.
   */
  const PGSTD::string &str() const;					//[t62]

private:
  size_type m_size;
  mutable PGSTD::string m_str;
};


/// Escape binary string for inclusion in SQL
PGSTD::string PQXX_LIBEXPORT escape_binary(const PGSTD::string &bin);
/// Escape binary string for inclusion in SQL
PGSTD::string PQXX_LIBEXPORT escape_binary(const char bin[]);
/// Escape binary string for inclusion in SQL
PGSTD::string PQXX_LIBEXPORT escape_binary(const char bin[], size_t len);
/// Escape binary string for inclusion in SQL
PGSTD::string PQXX_LIBEXPORT escape_binary(const unsigned char bin[]);
/// Escape binary string for inclusion in SQL
PGSTD::string PQXX_LIBEXPORT escape_binary(const unsigned char bin[], size_t len);


}

