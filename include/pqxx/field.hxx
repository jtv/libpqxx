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
 * Copyright (c) 2001-2017, Jeroen T. Vermeulen <jtv@xs4all.nl>
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

#if defined(PQXX_HAVE_OPTIONAL)
#include <optional>
#endif

#if defined(PQXX_HAVE_EXP_OPTIONAL)
#include <experimental/optional>
#endif

#include "pqxx/strconv"
#include "pqxx/types"


/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

namespace pqxx
{
/// Reference to a field in a result set.
/** A field represents one entry in a row.  It represents an actual value
 * in the result set, and can be converted to various types.
 */
class PQXX_LIBEXPORT field
{
public:
  typedef field_size_type size_type;

  /// Constructor.
  /** Create field as reference to a field in a result set.
   * @param R Row that this field is part of.
   * @param C Column number of this field.
   */
  field(const row &R, row_size_type C) PQXX_NOEXCEPT;		//[t1]

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

  row_size_type num() const { return col(); }				//[t82]

  /// What column number in its originating table did this column come from?
  row_size_type table_column() const;					//[t93]
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
  template<> bool to<std::string>(std::string &Obj) const;

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

#if defined(PQXX_HAVE_OPTIONAL)
  /// Return value as std::optional, or blank value if null.
  template<typename T> std::optional<T> get() const
	{ return get_opt<T, std::optional<T> >(); }
#endif

#if defined(PQXX_HAVE_EXP_OPTIONAL)
  /// Return value as std::experimental::optional, or blank value if null.
  template<typename T> std::experimental::optional<T> get() const
	{ return get_opt<T, std::experimental::optional<T> >(); }
#endif

  /// Is this field's value null?
  bool is_null() const PQXX_NOEXCEPT;					//[t12]

  /// Return number of bytes taken up by the field's value.
  /**
   * Includes the terminating zero byte.
   */
  size_type size() const PQXX_NOEXCEPT;					//[t11]
  //@}


protected:
  const result *home() const PQXX_NOEXCEPT { return m_home; }
  size_t idx() const PQXX_NOEXCEPT { return m_row; }
  row_size_type col() const PQXX_NOEXCEPT { return m_col; }

  row_size_type m_col;

private:
  /// Implementation for get().
#if defined(PQXX_HAVE_OPTIONAL) || defined(PQXX_HAVE_EXP_OPTIONAL)
  /**
   * Abstracts away the difference between std::optional and
   * std::experimental::optional.  Both can be supported at the same time,
   * so pre-C++17 code can still work once the compiler defaults to C++17.
   */
  template<typename T, typename OPTIONAL_T> OPTIONAL_T get_opt() const
  {
    if (is_null()) return OPTIONAL_T();
    else return OPTIONAL_T(as<T>());
  }
#endif

  const result *m_home;
  size_t m_row;
};


/// Specialization: <tt>to(string &)</tt>.
template<>
inline bool field::to<std::string>(std::string &Obj) const
{
  const char *const bytes = c_str();
  if (!bytes[0] && is_null()) return false;
  Obj = std::string(bytes, size());
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


template<typename CHAR=char, typename TRAITS=std::char_traits<CHAR> >
  class field_streambuf :
  public std::basic_streambuf<CHAR, TRAITS>
{
public:
  typedef CHAR char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;
  typedef std::ios::openmode openmode;
  typedef std::ios::seekdir seekdir;

  explicit field_streambuf(const field &F) :			//[t74]
    m_Field(F)
  {
    initialize();
  }

protected:
  virtual int sync() PQXX_OVERRIDE { return traits_type::eof(); }

protected:
  virtual pos_type seekoff(off_type, seekdir, openmode) PQXX_OVERRIDE
	{ return traits_type::eof(); }
  virtual pos_type seekpos(pos_type, openmode) PQXX_OVERRIDE
	{return traits_type::eof();}
  virtual int_type overflow(int_type) PQXX_OVERRIDE
	{ return traits_type::eof(); }
  virtual int_type underflow() PQXX_OVERRIDE
	{ return traits_type::eof(); }

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
template<typename CHAR=char, typename TRAITS=std::char_traits<CHAR> >
  class basic_fieldstream :
    public std::basic_istream<CHAR, TRAITS>
{
  typedef std::basic_istream<CHAR, TRAITS> super;

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
