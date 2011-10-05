/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/field.hxx
 *
 *   DESCRIPTION
 *      definitions for the pqxx::field class.
 *   pqxx::field refers to a field in a query result.
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/field instead.
 *
 * Copyright (c) 2001-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_FIELD
#define PQXX_H_FIELD

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/strconv"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{
class result;
class tuple;

typedef unsigned int tuple_size_type;
typedef signed int tuple_difference_type;

/// Reference to a field in a result set.
/** A field represents one entry in a tuple.  It represents an actual value
 * in the result set, and can be converted to various types.
 */
class PQXX_LIBEXPORT field
{
public:
  typedef size_t size_type;

  /// Constructor.
  /** Create field as reference to a field in a result set.
   * @param T Tuple that this field is part of.
   * @param C Column number of this field.
   */
  field(const tuple &T, tuple_size_type C) throw ();		//[t1]

  /**
   * @name Comparison
   */
  //@{
  /// Byte-by-byte comparison of two fields (all nulls are considered equal)
  /** @warning null handling is still open to discussion and change!
   *
   * Handling of null values differs from that in SQL where a comparison
   * involving a null value yields null, so nulls are never considered equal
   * to one another or even to themselves.
   *
   * Null handling also probably differs from the closest equivalent in C++,
   * which is the NaN (Not-a-Number) value, a singularity comparable to
   * SQL's null.  This is because the builtin == operator demands that a == a.
   *
   * The usefulness of this operator is questionable.  No interpretation
   * whatsoever is imposed on the data; 0 and 0.0 are considered different,
   * as are null vs. the empty string, or even different (but possibly
   * equivalent and equally valid) encodings of the same Unicode character
   * etc.
   */
  bool operator==(const field &) const;				//[t75]

  /// Byte-by-byte comparison (all nulls are considered equal)
  /** @warning See operator==() for important information about this operator
   */
  bool operator!=(const field &rhs) const				//[t82]
						    {return !operator==(rhs);}
  //@}

  /**
   * @name Column information
   */
  //@{
  /// Column name
  const char *name() const;						//[t11]

  /// Column type
  oid type() const;							//[t7]

  /// What table did this column come from?
  oid table() const;							//[t2]

  tuple_size_type num() const { return col(); }				//[t82]

  /// What column number in its originating table did this column come from?
  tuple_size_type table_column() const;					//[t93]
  //@}

  /**
   * @name Content access
   */
  //@{
  /// Read as plain C string
  /** Since the field's data is stored internally in the form of a
   * zero-terminated C string, this is the fastest way to read it.  Use the
   * to() or as() functions to convert the string to other types such as
   * @c int, or to C++ strings.
   */
  const char *c_str() const;						//[t2]

  /// Read value into Obj; or leave Obj untouched and return @c false if null
  template<typename T> bool to(T &Obj) const				//[t3]
  {
    const char *const bytes = c_str();
    if (!bytes[0] && is_null()) return false;
    from_string(bytes, Obj);
    return true;
  }

  /// Read value into Obj; or leave Obj untouched and return @c false if null
  template<typename T> bool operator>>(T &Obj) const			//[t7]
      { return to(Obj); }

#ifdef PQXX_NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION
  /// Specialization: to(string &)
  template<> bool to<PGSTD::string>(PGSTD::string &Obj) const;

  /// Specialization: <tt>to(const char *&)</tt>.
  /** The buffer has the same lifetime as the result, so take care not to
   * use it after the result is destroyed.
   */
  template<> bool to<const char *>(const char *&Obj) const;
#endif

  /// Read value into Obj; or use Default & return @c false if null
  template<typename T> bool to(T &Obj, const T &Default) const	//[t12]
  {
    const bool NotNull = to(Obj);
    if (!NotNull) Obj = Default;
    return NotNull;
  }

  /// Return value as object of given type, or Default if null
  /** Note that unless the function is instantiated with an explicit template
   * argument, the Default value's type also determines the result type.
   */
  template<typename T> T as(const T &Default) const			//[t1]
  {
    T Obj;
    to(Obj, Default);
    return Obj;
  }

  /// Return value as object of given type, or throw exception if null
  template<typename T> T as() const					//[t45]
  {
    T Obj;
    const bool NotNull = to(Obj);
    if (!NotNull) Obj = string_traits<T>::null();
    return Obj;
  }

  bool is_null() const throw ();					//[t12]
  size_type size() const throw ();					//[t11]
  //@}


protected:
  const result *home() const throw () { return m_home; }
  size_t idx() const throw () { return m_row; }
  tuple_size_type col() const throw () { return m_col; }

  tuple_size_type m_col;

private:
  const result *m_home;
  size_t m_row;
};


/// Specialization: <tt>to(string &)</tt>.
template<>
inline bool field::to<PGSTD::string>(PGSTD::string &Obj) const
{
  const char *const bytes = c_str();
  if (!bytes[0] && is_null()) return false;
  Obj = PGSTD::string(bytes, size());
  return true;
}

/// Specialization: <tt>to(const char *&)</tt>.
/** The buffer has the same lifetime as the data in this result (i.e. of this
 * result object, or the last remaining one copied from it etc.), so take care
 * not to use it after the last result object referring to this query result is
 * destroyed.
 */
template<>
inline bool field::to<const char *>(const char *&Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}


template<typename CHAR=char, typename TRAITS=PGSTD::char_traits<CHAR> >
  class field_streambuf :
#ifdef PQXX_HAVE_STREAMBUF
  public PGSTD::basic_streambuf<CHAR, TRAITS>
#else
  public PGSTD::streambuf
#endif
{
public:
  typedef CHAR char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
#ifdef PQXX_HAVE_STREAMBUF
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;
#else
  typedef streamoff off_type;
  typedef streampos pos_type;
#endif
  typedef PGSTD::ios::openmode openmode;
  typedef PGSTD::ios::seekdir seekdir;

  explicit field_streambuf(const field &F) :			//[t74]
    m_Field(F)
  {
    initialize();
  }

#ifdef PQXX_HAVE_STREAMBUF
protected:
#endif
  virtual int sync() { return traits_type::eof(); }

protected:
  virtual pos_type seekoff(off_type, seekdir, openmode)
	{ return traits_type::eof(); }
  virtual pos_type seekpos(pos_type, openmode) {return traits_type::eof();}
  virtual int_type overflow(int_type) { return traits_type::eof(); }
  virtual int_type underflow() { return traits_type::eof(); }

private:
  const field &m_Field;

  int_type initialize()
  {
    char_type *G =
      reinterpret_cast<char_type *>(const_cast<char *>(m_Field.c_str()));
    this->setg(G, G, G + m_Field.size());
    return int_type(m_Field.size());
  }
};


/// Input stream that gets its data from a result field
/** Use this class exactly as you would any other istream to read data from a
 * field.  All formatting and streaming operations of @c std::istream are
 * supported.  What you'll typically want to use, however, is the fieldstream
 * typedef (which defines a basic_fieldstream for @c char).  This is similar to
 * how e.g. @c std::ifstream relates to @c std::basic_ifstream.
 *
 * This class has only been tested for the char type (and its default traits).
 */
template<typename CHAR=char, typename TRAITS=PGSTD::char_traits<CHAR> >
  class basic_fieldstream :
#ifdef PQXX_HAVE_STREAMBUF
    public PGSTD::basic_istream<CHAR, TRAITS>
#else
    public PGSTD::istream
#endif
{
#ifdef PQXX_HAVE_STREAMBUF
  typedef PGSTD::basic_istream<CHAR, TRAITS> super;
#else
  typedef PGSTD::istream super;
#endif

public:
  typedef CHAR char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;

  basic_fieldstream(const field &F) : super(0), m_Buf(F)
	{ super::init(&m_Buf); }

private:
  field_streambuf<CHAR, TRAITS> m_Buf;
};

typedef basic_fieldstream<char> fieldstream;

} // namespace pqxx


#include "pqxx/compiler-internal-post.hxx"

#endif
