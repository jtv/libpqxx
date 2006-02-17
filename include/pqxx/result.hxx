/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/result.hxx
 *
 *   DESCRIPTION
 *      definitions for the pqxx::result class and support classes.
 *   pqxx::result represents the set of result tuples from a database query
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/result instead.
 *
 * Copyright (c) 2001-2006, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-public.hxx"

#ifdef PQXX_HAVE_IOS
#include <ios>
#endif

#include <stdexcept>

#include "pqxx/util"

/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

// TODO: Support SQL arrays

namespace pqxx
{
/// Query or command result set.
/** This behaves as a container (as defined by the C++ standard library) and
 * provides random access const iterators to iterate over its tuples.  A tuple
 * can also be accessed by indexing a result R by the tuple's zero-based
 * number:
 *
 * @code
 *	for (result::size_type i=0; i < R.size(); ++i) Process(R[i]);
 * @endcode
 *
 * Result sets in libpqxx are lightweight, reference-counted wrapper objects
 * (following the Proxy design pattern) that are small and cheap to copy.  Think
 * of a result object as a "smart pointer" to an underlying result set.
 *
 * @warning The result set that a result object points to is not thread-safe.
 * If you copy a result object, it still refers to the same underlying result
 * set.  So never copy, destroy, query, or otherwise access a result while
 * another thread may be copying, destroying, querying, or otherwise accessing
 * the same result set--even if it is doing so through a different result
 * object!
 */
class PQXX_LIBEXPORT result : private internal::PQAlloc<internal::pq::PGresult>
{
  typedef internal::PQAlloc<internal::pq::PGresult> super;
public:
  class const_iterator;
  class const_fielditerator;
  class const_reverse_fielditerator;
  class tuple;
  class field;
  typedef unsigned long size_type;
  typedef signed long difference_type;
  typedef tuple reference;
  typedef const_iterator pointer;

  /// Reference to one row in a result.
  /** A tuple represents one row (also called a tuple) in a query result set.
   * It also acts as a container mapping column numbers or names to field
   * values (see below):
   *
   * @code
   * 	cout << tuple["date"].c_str() << ": " << tuple["name"].c_str() << endl;
   * @endcode
   *
   * The tuple itself acts like a (non-modifyable) container, complete with its
   * own const_iterator and const_reverse_iterator.
   */
  class PQXX_LIBEXPORT tuple
  {
  public:
    typedef unsigned int size_type;
    typedef signed int difference_type;
    typedef const_fielditerator const_iterator;
    typedef field reference;
    typedef const_fielditerator pointer;
    typedef const_reverse_fielditerator const_reverse_iterator;

    tuple(const result *r, result::size_type i) throw () :
      m_Home(r), m_Index(i) {}
    ~tuple() throw () {} // Yes Scott Meyers, you're absolutely right[1]

    /**
     * @name Comparison
     */
    //@{
    bool operator==(const tuple &) const throw ();			//[t75]
    bool operator!=(const tuple &rhs) const throw ()			//[t75]
	{ return !operator==(rhs); }
    //@}

    const_iterator begin() const throw ()				//[t82]
	{ return const_iterator(*this, 0); }
    const_iterator end() const throw ()					//[t82]
	{ return const_iterator(*this, size()); }

    /**
     * @name Field access
     */
    //@{
    reference front() const throw () { return field(*this, 0); }	//[t74]
    reference back() const throw () { return field(*this, size()-1); }	//[t75]

    const_reverse_fielditerator rbegin() const;				//[t82]
    const_reverse_fielditerator rend() const;				//[t82]

    reference operator[](size_type i) const throw ()			//[t11]
	{ return field(*this, i); }
    reference operator[](int i) const throw ()				//[t2]
	{ return operator[](size_type(i)); }
    reference operator[](const char[]) const;				//[t11]
    reference operator[](const PGSTD::string &s) const			//[t11]
	{ return operator[](s.c_str()); }
    reference at(size_type) const throw (PGSTD::out_of_range);		//[t11]
    reference at(int i) const throw (PGSTD::out_of_range)		//[t11]
	{ return at(size_type(i)); }
    reference at(const char[]) const;					//[t11]
    reference at(const PGSTD::string &s) const				//[t11]
	{ return at(s.c_str()); }
    //@}

    size_type size() const throw () { return m_Home->columns(); }	//[t11]

    void swap(tuple &) throw ();					//[t11]

    result::size_type rownumber() const throw () { return m_Index; }	//[t11]

    /**
     * @name Column information
     */
    //@{
    /// Number of given column (throws exception if it doesn't exist)
    size_type column_number(const PGSTD::string &ColName) const		//[t30]
	{ return m_Home->column_number(ColName); }

    /// Number of given column (throws exception if it doesn't exist)
    size_type column_number(const char ColName[]) const			//[t30]
	{ return m_Home->column_number(ColName); }

    /// Type of given column
    oid column_type(size_type ColNum) const				//[t7]
	{ return m_Home->column_type(ColNum); }

    /// Type of given column
    oid column_type(int ColNum) const					//[t7]
	{ return column_type(size_type(ColNum)); }

    /// Type of given column
    oid column_type(const PGSTD::string &ColName) const			//[t7]
	{ return column_type(column_number(ColName)); }

    /// Type of given column
    oid column_type(const char ColName[]) const				//[t7]
	{ return column_type(column_number(ColName)); }

    /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
    /** Only defined if the libpqxx library was compiled against a libpq
     * version that supports the @c PQftable function.
     *
     * If you get a link error saying this function is undefined, that must be
     * because libpqxx was compiled against an older version of libpq.  The
     * @c PQftable function first became available in PostgreSQL 7.4.
     */
    oid column_table(size_type ColNum) const				//[t2]
	{ return m_Home->column_table(ColNum); }
    /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
    /** Only defined if the libpqxx library was compiled against a libpq
     * version that supports the @c PQftable function.
     *
     * If you get a link error saying this function is undefined, that must be
     * because libpqxx was compiled against an older version of libpq.  The
     * @c PQftable function first became available in PostgreSQL 7.4.
     */
    oid column_table(int ColNum) const					//[t2]
	{ return column_table(size_type(ColNum)); }
    /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
    /** Only defined if the libpqxx library was compiled against a libpq
     * version that supports the @c PQftable function.
     *
     * If you get a link error saying this function is undefined, that must be
     * because libpqxx was compiled against an older version of libpq.  The
     * @c PQftable function first became available in PostgreSQL 7.4.
     */
    oid column_table(const PGSTD::string &ColName) const		//[t2]
	{ return column_table(column_number(ColName)); }
    //@}

    result::size_type num() const { return rownumber(); }		//[t1]


#ifdef PQXX_DEPRECATED_HEADERS
    /**
     * @name 1.x API
     */
    //@{
    /// @deprecated Use rownumber() instead
    result::size_type Row() const { return rownumber(); }

    /// @deprecated Use column_number() instead
    size_type ColumnNumber(const PGSTD::string &ColName) const
	{ return column_number(ColName); }

    /// @deprecated Use column_number() instead
    size_type ColumnNumber(const char ColName[]) const
	{ return column_number(ColName); }
    //@}
#endif

  protected:
    friend class field;
    const result *m_Home;
    result::size_type m_Index;

    // Not allowed:
    tuple();
  };

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
    field(const tuple &T, tuple::size_type C) throw () : 		//[t1]
	m_tup(T), m_col(C) {}

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
    bool operator!=(const field &rhs) const {return !operator==(rhs);}	//[t82]
    //@}

    /**
     * @name Column information
     */
    //@{
    /// Column name
    const char *name() const { return home()->column_name(col()); }	//[t11]

    /// Column type
    oid type() const { return home()->column_type(col()); } 		//[t7]

    /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
    /** Only defined if the libpqxx library was compiled against a libpq
     * version that supports the @c PQftable function.
     *
     * If you get a link error saying this function is undefined, that must be
     * because libpqxx was compiled against an older version of libpq.  The
     * @c PQftable function first became available in PostgreSQL 7.4.
     */
    oid table() const { return home()->column_table(col()); }		//[t2]

    tuple::size_type num() const { return col(); }			//[t82]
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
    const char *c_str() const { return home()->GetValue(idx(),col()); }	//[t2]

    /// Read value into Obj; or leave Obj untouched and return @c false if null
    template<typename T> bool to(T &Obj) const				//[t3]
    {
      if (is_null()) return false;
      try
      {
        from_string(c_str(), Obj);
      }
      catch (const PGSTD::exception &e)
      {
        throw PGSTD::domain_error("Error reading field " +
			          PGSTD::string(name()) + ": " +
				  e.what());
      }
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
      if (!NotNull) throw PGSTD::domain_error("Attempt to read null field");
      return Obj;
    }

    bool is_null() const { return home()->GetIsNull(idx(), col()); }	//[t12]
    size_type size() const throw ()					//[t11]
	{ return home()->GetLength(idx(),col()); }
    //@}

#ifdef PQXX_DEPRECATED_HEADERS
    /// @deprecated Use name() instead
    const char *Name() const {return name();}
#endif

  private:
    const result *home() const throw () { return m_tup.m_Home; }
    result::size_type idx() const throw () { return m_tup.m_Index; }

  protected:
    const tuple::size_type col() const throw () { return m_col; }
    tuple m_tup;
    tuple::size_type m_col;
  };

  typedef PGSTD::iterator<PGSTD::random_access_iterator_tag,
                           const tuple,
                           result::difference_type,
    			   const_iterator,
    			   tuple> const_iterator_base;

  /// Iterator for rows (tuples) in a query result set.
  /** A result, once obtained, cannot be modified.  Therefore there is no
   * plain iterator type for result.  However its const_iterator type can be
   * used to inspect its tuples without changing them.
   */
  class PQXX_LIBEXPORT const_iterator :
    public const_iterator_base,
    public tuple
  {
  public:
    typedef const tuple *pointer;
    typedef tuple reference;
    typedef result::size_type size_type;
    typedef result::difference_type difference_type;

    const_iterator() throw () : tuple(0,0) {}
    const_iterator(const tuple &t) throw () : tuple(t) {}

    /**
     * @name Dereferencing operators
     */
    //@{
    /** The iterator "points to" its own tuple, which is also itself.  This
     * allows a result to be addressed as a two-dimensional container without
     * going through the intermediate step of dereferencing the iterator.  I
     * hope this works out to be similar to C pointer/array semantics in useful
     * cases.
     *
     * IIRC Alex Stepanov, the inventor of the STL, once remarked that having
     * this as standard behaviour for pointers would be useful in some
     * algorithms.  So even if this makes me look foolish, I would seem to be in
     * distinguished company.
     */
    pointer operator->() const { return this; }				//[t12]
    reference operator*() const { return tuple(*this); }		//[t12]
    //@}

    /**
     * @name Manipulations
     */
    //@{
    const_iterator operator++(int);					//[t12]
    const_iterator &operator++() { ++m_Index; return *this; }		//[t1]
    const_iterator operator--(int);					//[t12]
    const_iterator &operator--() { --m_Index; return *this; }		//[t12]

    const_iterator &operator+=(difference_type i) 			//[t12]
	{ m_Index+=i; return *this; }
    const_iterator &operator-=(difference_type i) 			//[t12]
	{ m_Index-=i; return *this; }
    //@}

    /**
     * @name Comparisons
     */
    //@{
    bool operator==(const const_iterator &i) const 			//[t12]
	{return m_Index==i.m_Index;}
    bool operator!=(const const_iterator &i) const 			//[t12]
	{return m_Index!=i.m_Index;}
    bool operator<(const const_iterator &i) const   			//[t12]
 	{return m_Index<i.m_Index;}
    bool operator<=(const const_iterator &i) const 			//[t12]
	{return m_Index<=i.m_Index;}
    bool operator>(const const_iterator &i) const   			//[t12]
	{return m_Index>i.m_Index;}
    bool operator>=(const const_iterator &i) const 			//[t12]
	{return m_Index>=i.m_Index;}
    //@}

    /**
     * @name Arithmetic operators
     */
    //@{
    inline const_iterator operator+(difference_type) const;		//[t12]
    friend const_iterator
    operator+(difference_type, const_iterator);				//[t12]
    inline const_iterator operator-(difference_type) const;		//[t12]
    inline difference_type operator-(const_iterator) const;		//[t12]
    //@}

  private:
    friend class pqxx::result;
    const_iterator(const pqxx::result *r, result::size_type i) throw () :
  	tuple(r, i) {}
  };

  class PQXX_LIBEXPORT const_reverse_iterator : private const_iterator
  {
  public:
    typedef pqxx::result::const_iterator super;
    typedef pqxx::result::const_iterator iterator_type;
    using iterator_type::iterator_category;
    using iterator_type::difference_type;
    using iterator_type::pointer;
#ifndef _MSC_VER
    using iterator_type::value_type;
    using iterator_type::reference;
#else
    // Workaround for Visual C++.NET 2003, which has access problems
    typedef const tuple &reference;
    typedef tuple value_type;
#endif

    const_reverse_iterator(const const_reverse_iterator &rhs) :		//[t75]
      const_iterator(rhs) {}
    explicit const_reverse_iterator(const const_iterator &rhs) : 	//[t75]
      const_iterator(rhs) { super::operator--(); }

    iterator_type base() const throw ();				//[t75]

    /**
     * @name Dereferencing operators
     */
    //@{
    using const_iterator::operator->;	 				//[t75]
    using const_iterator::operator*;					//[t75]
    //@}
 
    /**
     * @name Manipulations
     */
    //@{
    const_reverse_iterator &operator=(const const_reverse_iterator &r)	//[t75]
	{ iterator_type::operator=(r); return *this; }
    const_reverse_iterator operator++()					//[t75]
	{ iterator_type::operator--(); return *this; }
    const_reverse_iterator operator++(int);				//[t75]
    const_reverse_iterator &operator--() 				//[t75]
	{ iterator_type::operator++(); return *this; }
    const_reverse_iterator operator--(int);				//[t75]
    const_reverse_iterator &operator+=(difference_type i)		//[t75]
	{ iterator_type::operator-=(i); return *this; }
    const_reverse_iterator &operator-=(difference_type i)		//[t75]
	{ iterator_type::operator+=(i); return *this; }
    //@}

    /**
     * @name Arithmetic operators
     */
    //@{
    const_reverse_iterator operator+(difference_type i) const		//[t75]
	{ return const_reverse_iterator(base()-i); }
    const_reverse_iterator operator-(difference_type i)			//[t75]
	{ return const_reverse_iterator(base()+i); }
    difference_type operator-(const const_reverse_iterator &rhs) const	//[t75]
	{ return rhs.const_iterator::operator-(*this); }
    //@}

    /**
     * @name Comparisons
     */
    //@{
    bool operator==(const const_reverse_iterator &rhs) const throw ()	//[t75]
	{ return iterator_type::operator==(rhs); }
    bool operator!=(const const_reverse_iterator &rhs) const throw ()	//[t75]
	{ return !operator==(rhs); }

    bool operator<(const const_reverse_iterator &rhs) const 		//[t75]
	{ return iterator_type::operator>(rhs); }
    bool operator<=(const const_reverse_iterator &rhs) const 		//[t75]
	{ return iterator_type::operator>=(rhs); }
    bool operator>(const const_reverse_iterator &rhs) const 		//[t75]
	{ return iterator_type::operator<(rhs); }
    bool operator>=(const const_reverse_iterator &rhs) const 		//[t75]
	{ return iterator_type::operator<=(rhs); }
    //@}
  };

  class PQXX_LIBEXPORT const_fielditerator :
    public PGSTD::iterator<PGSTD::random_access_iterator_tag,
                           const field,
                           tuple::size_type>,
    public field
  {
    typedef PGSTD::iterator<PGSTD::random_access_iterator_tag,
  				  const field,
				  tuple::size_type> it;
  public:
    using it::pointer;
    typedef tuple::size_type size_type;
    typedef tuple::difference_type difference_type;
    typedef field reference;

    const_fielditerator(const tuple &T, tuple::size_type C) throw () :	//[t82]
      field(T, C) {}
    const_fielditerator(const field &F) throw () : field(F) {}		//[t82]

    /**
     * @name Dereferencing operators
     */
    //@{
    pointer operator->() const { return this; }				//[t82]
    reference operator*() const { return field(*this); }		//[t82]
    //@}

    /**
     * @name Manipulations
     */
    //@{
    const_fielditerator operator++(int);				//[t82]
    const_fielditerator &operator++() { ++m_col; return *this; }	//[t82]
    const_fielditerator operator--(int);				//[t82]
    const_fielditerator &operator--() { --m_col; return *this; }	//[t82]

    const_fielditerator &operator+=(difference_type i) 			//[t82]
	{ m_col+=i; return *this; }
    const_fielditerator &operator-=(difference_type i) 			//[t82]
	{ m_col-=i; return *this; }
    //@}

    /**
     * @name Comparisons
     */
    //@{
    bool operator==(const const_fielditerator &i) const 		//[t82]
	{return col()==i.col();}
    bool operator!=(const const_fielditerator &i) const 		//[t82]
	{return col()!=i.col();}
    bool operator<(const const_fielditerator &i) const  		//[t82]
 	{return col()<i.col();}
    bool operator<=(const const_fielditerator &i) const 		//[t82]
	{return col()<=i.col();}
    bool operator>(const const_fielditerator &i) const   		//[t82]
	{return col()>i.col();}
    bool operator>=(const const_fielditerator &i) const 		//[t82]
	{return col()>=i.col();}
    //@}

    /**
     * @name Arithmetic operators
     */
    //@{
    inline const_fielditerator operator+(difference_type) const;	//[t82]

    friend const_fielditerator operator+(difference_type,
		    		          const_fielditerator);		//[t82]

    inline const_fielditerator operator-(difference_type) const;	//[t82]
    inline difference_type operator-(const_fielditerator) const;	//[t82]
    //@}
  };

  class PQXX_LIBEXPORT const_reverse_fielditerator : private const_fielditerator
  {
  public:
    typedef const_fielditerator super;
    typedef const_fielditerator iterator_type;
    using iterator_type::iterator_category;
    using iterator_type::difference_type;
    using iterator_type::pointer;
#ifndef _MSC_VER
    using iterator_type::value_type;
    using iterator_type::reference;
#else
    // Workaround for Visual C++.NET 2003, which has access problems
    typedef field value_type;
    typedef const field &reference;
#endif

    const_reverse_fielditerator(const const_reverse_fielditerator &r) :	//[t82]
      const_fielditerator(r) {}
    explicit
      const_reverse_fielditerator(const super &rhs) throw() :		//[t82]
	const_fielditerator(rhs) { super::operator--(); }

    iterator_type base() const throw ();				//[t82]

    /**
     * @name Dereferencing operators
     */
    //@{
    using iterator_type::operator->;					//[t82]
    using iterator_type::operator*;					//[t82]
    //@}

    /**
     * @name Manipulations
     */
    //@{
    const_reverse_fielditerator &
      operator=(const const_reverse_fielditerator &r)			//[t82]
	{ iterator_type::operator=(r); return *this; }
    const_reverse_fielditerator operator++()				//[t82]
	{ iterator_type::operator--(); return *this; }
    const_reverse_fielditerator operator++(int);			//[t82]
    const_reverse_fielditerator &operator--() 				//[t82]
	{ iterator_type::operator++(); return *this; }
    const_reverse_fielditerator operator--(int);			//[t82]
    const_reverse_fielditerator &operator+=(difference_type i)		//[t82]
	{ iterator_type::operator-=(i); return *this; }
    const_reverse_fielditerator &operator-=(difference_type i)		//[t82]
	{ iterator_type::operator+=(i); return *this; }
    //@}

    /**
     * @name Arithmetic operators
     */
    //@{
    const_reverse_fielditerator operator+(difference_type i) const	//[t82]
	{ return const_reverse_fielditerator(base()-i); }
    const_reverse_fielditerator operator-(difference_type i)		//[t82]
	{ return const_reverse_fielditerator(base()+i); }
    difference_type
      operator-(const const_reverse_fielditerator &rhs) const		//[t82]
	{ return rhs.const_fielditerator::operator-(*this); }
    //@}

    /**
     * @name Comparisons
     */
    //@{
    bool
      operator==(const const_reverse_fielditerator &rhs) const throw ()	//[t82]
	{ return iterator_type::operator==(rhs); }
    bool
      operator!=(const const_reverse_fielditerator &rhs) const throw ()	//[t82]
	{ return !operator==(rhs); }

    bool operator<(const const_reverse_fielditerator &rhs) const 	//[t82]
	{ return iterator_type::operator>(rhs); }
    bool operator<=(const const_reverse_fielditerator &rhs) const 	//[t82]
	{ return iterator_type::operator>=(rhs); }
    bool operator>(const const_reverse_fielditerator &rhs) const 	//[t82]
	{ return iterator_type::operator<(rhs); }
    bool operator>=(const const_reverse_fielditerator &rhs) const 	//[t82]
	{ return iterator_type::operator<=(rhs); }
    //@}
  };


  result() throw () : super() {}					//[t3]
  result(const result &rhs) throw () : super(rhs) {}			//[t1]

  result &operator=(const result &rhs) throw ()				//[t10]
  	{ super::operator=(rhs); return *this; }

  /**
   * @name Comparisons
   */
  //@{
  bool operator==(const result &) const throw ();			//[t70]
  bool operator!=(const result &rhs) const throw ()			//[t70]
  	{ return !operator==(rhs); }
  //@}

  const_reverse_iterator rbegin() const 				//[t75]
  	{ return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const					//[t75]
   	{ return const_reverse_iterator(begin()); }

  const_iterator begin() const throw ()					//[t1]
  	{ return const_iterator(this, 0); }
  inline const_iterator end() const throw ();				//[t1]

  reference front() const throw () { return tuple(this,0); }		//[t74]
  reference back() const throw () {return tuple(this,size()-1);}	//[t75]

  size_type size() const throw ();					//[t2]
  bool empty() const throw ();						//[t11]
  size_type capacity() const throw () { return size(); }		//[t20]

  void swap(result &) throw ();						//[t77]

  const tuple operator[](size_type i) const throw () 			//[t2]
  	{ return tuple(this, i); }
  const tuple at(size_type) const throw (PGSTD::out_of_range);		//[t10]

  using super::clear;							//[t20]

  /**
   * @name Column information
   */
  //@{
  /// Number of columns in result
  tuple::size_type columns() const throw (); 				//[t11]

  /// Number of given column (throws exception if it doesn't exist)
  tuple::size_type column_number(const char ColName[]) const;		//[t11]

  /// Number of given column (throws exception if it doesn't exist)
  tuple::size_type column_number(const PGSTD::string &Name) const	//[t11]
  	{return column_number(Name.c_str());}

  /// Name of column with this number (throws exception if it doesn't exist)
  const char *column_name(tuple::size_type Number) const;		//[t11]

  /// Type of given column
  oid column_type(tuple::size_type ColNum) const;			//[t7]
  /// Type of given column
  oid column_type(int ColNum) const					//[t7]
  	{ return column_type(tuple::size_type(ColNum)); }

  /// Type of given column
  oid column_type(const PGSTD::string &ColName) const			//[t7]
    	{ return column_type(column_number(ColName)); }

  /// Type of given column
  oid column_type(const char ColName[]) const				//[t7]
    	{ return column_type(column_number(ColName)); }

  /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
  /** Only defined if the libpqxx library was compiled against a libpq
   * version that supports the @c PQftable function.
   *
   * If you get a link error saying this function is undefined, that must be
   * because libpqxx was compiled against an older version of libpq.  The
   * @c PQftable function first became available in PostgreSQL 7.4.
   */
  oid column_table(tuple::size_type ColNum) const;			//[t2]

  /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
  /** Only defined if the libpqxx library was compiled against a libpq
   * version that supports the @c PQftable function.
   *
   * If you get a link error saying this function is undefined, that must be
   * because libpqxx was compiled against an older version of libpq.  The
   * @c PQftable function first became available in PostgreSQL 7.4.
   */
  oid column_table(int ColNum) const					//[t2]
  	{ return column_table(tuple::size_type(ColNum)); }

  /// What table did this column come from?  Requires PostgreSQL 7.4 C API.
  /** Only defined if the libpqxx library was compiled against a libpq
   * version that supports the @c PQftable function.
   *
   * If you get a link error saying this function is undefined, that must be
   * because libpqxx was compiled against an older version of libpq.  The
   * @c PQftable function first became available in PostgreSQL 7.4.
   */
  oid column_table(const PGSTD::string &ColName) const			//[t2]
    	{ return column_table(column_number(ColName)); }
  //@}

  /// If command was @c INSERT of 1 row, return oid of inserted row
  /** @return Identifier of inserted row if exactly one row was inserted, or
   * oid_none otherwise.
   */
  oid inserted_oid() const;						//[t13]


  /// If command was @c INSERT, @c UPDATE, or @c DELETE: number of affected rows
  /** @return Number of affected rows if last command was @c INSERT, @c UPDATE,
   * or @c DELETE; zero for all other commands.
   */
  size_type affected_rows() const;					//[t7]


#ifdef PQXX_DEPRECATED_HEADERS
  /**
   * @name 1.x API
   */
  //@{
  /// @deprecated For compatibility with old Tuple class
  typedef tuple Tuple;
  /// @deprecated For compatibility with old Field class
  typedef field Field;
  /// @deprecated Use inserted_oid() instead
  oid InsertedOid() const { return inserted_oid(); }
  /// @deprecated Use affected_rows() instead
  size_type AffectedRows() const { return affected_rows(); }
  /// @deprecated Use columns() instead
  tuple::size_type Columns() const { return columns(); }
  /// @deprecated Use column_number() instead
  tuple::size_type ColumnNumber(const char Name[]) const
  	{return column_number(Name);}
  /// @deprecated Use column_number() instead
  tuple::size_type ColumnNumber(const PGSTD::string &Name) const
  	{return column_number(Name);}
  /// @deprecated Use column_name() instead
  const char *ColumnName(tuple::size_type Number) const
  	{return column_name(Number);}
  //@}
#endif


private:
  friend class pqxx::result::field;
  const char *GetValue(size_type Row, tuple::size_type Col) const;
  bool GetIsNull(size_type Row, tuple::size_type Col) const;
  field::size_type GetLength(size_type, tuple::size_type) const;

  friend class connection_base;
  friend class pipeline;
  explicit result(internal::pq::PGresult *rhs) throw () : super(rhs) {}
  result &operator=(internal::pq::PGresult *rhs) throw ()
  	{ super::operator=(rhs); return *this; }
  bool operator!() const throw () { return !c_ptr(); }
  operator bool() const throw () { return c_ptr() != 0; }
  void PQXX_PRIVATE CheckStatus(const PGSTD::string &Query) const;
  void PQXX_PRIVATE CheckStatus(const char Query[]) const;
  int PQXX_PRIVATE errorposition() const throw ();
  PGSTD::string PQXX_PRIVATE StatusError() const;

  friend class Cursor;	// deprecated
  friend class cursor_base;
  const char *CmdStatus() const throw ();
};


/// Write a result field to any type of stream
/** This can be convenient when writing a field to an output stream.  More
 * importantly, it lets you write a field to e.g. a @c stringstream which you
 * can then use to read, format and convert the field in ways that to() does not
 * support.
 *
 * Example: parse a field into a variable of the nonstandard
 * "<tt>long long</tt>" type.
 *
 * @code
 * extern result R;
 * long long L;
 * stringstream S;
 *
 * // Write field's string into S
 * S << R[0][0];
 *
 * // Parse contents of S into L
 * S >> L;
 * @endcode
 */
template<typename STREAM>
inline STREAM &operator<<(STREAM &S, const pqxx::result::field &F)	//[t46]
{
  S.write(F.c_str(), F.size());
  return S;
}


/// Convert a field's string contents to another type
template<typename T>
inline void from_string(const result::field &F, T &Obj)			//[t46]
	{ from_string(F.c_str(), Obj); }

/// Convert a field to a string
template<>
inline PGSTD::string to_string(const result::field &Obj)		//[t74]
	{ return to_string(Obj.c_str()); }


/// Specialization: <tt>to(string &)</tt>.
template<>
inline bool result::field::to<PGSTD::string>(PGSTD::string &Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}

/// Specialization: <tt>to(const char *&)</tt>.
/** The buffer has the same lifetime as the data in this result (i.e. of this
 * result object, or the last remaining one copied from it etc.), so take care
 * not to use it after the last result object referring to this query result is
 * destroyed.
 */
template<>
inline bool result::field::to<const char *>(const char *&Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}


inline result::tuple::const_reverse_iterator result::tuple::rbegin() const
	{ return const_reverse_fielditerator(end()); }
inline result::tuple::const_reverse_iterator result::tuple::rend() const
	{ return const_reverse_fielditerator(begin()); }

inline result::const_iterator
result::const_iterator::operator+(difference_type o) const
	{ return const_iterator(m_Home, m_Index + o); }

inline result::const_iterator
operator+(result::const_iterator::difference_type o, result::const_iterator i)
	{ return i + o; }

inline result::const_iterator
result::const_iterator::operator-(difference_type o) const
	{ return const_iterator(m_Home, m_Index - o); }

inline result::const_iterator::difference_type
result::const_iterator::operator-(const_iterator i) const
	{ return num()-i.num(); }

inline result::const_iterator result::end() const throw ()
	{ return const_iterator(this, size()); }


inline result::const_reverse_iterator
operator+(result::const_reverse_iterator::difference_type n,
	  const result::const_reverse_iterator &i)
	{ return result::const_reverse_iterator(i.base() - n); }

inline result::const_fielditerator
result::const_fielditerator::operator+(difference_type o) const
	{ return const_fielditerator(m_tup, col() + o); }

inline result::const_fielditerator
operator+(result::const_fielditerator::difference_type o,
	  result::const_fielditerator i)
	{ return i + o; }

inline result::const_fielditerator
result::const_fielditerator::operator-(difference_type o) const
	{ return const_fielditerator(m_tup, col() - o); }

inline result::const_fielditerator::difference_type
result::const_fielditerator::operator-(const_fielditerator i) const
	{ return num()-i.num(); }


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

  explicit field_streambuf(const result::field &F) : 			//[t74]
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
  const result::field &m_Field;

  int_type initialize()
  {
    char_type *G =
      reinterpret_cast<char_type *>(const_cast<char *>(m_Field.c_str()));
    setg(G, G, G + m_Field.size());
    return m_Field.size();
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

  basic_fieldstream(const result::field &F) : super(0), m_Buf(F)
	{ super::init(&m_Buf); }

private:
  field_streambuf<CHAR, TRAITS> m_Buf;
};

typedef basic_fieldstream<char> fieldstream;

} // namespace pqxx



/*
[1] Scott Meyers, in one of his essential books, "Effective C++" and "More
Effective C++", points out that it is good style to have any class containing
a member of pointer type define a destructor--just to show that it knows what it
is doing with the pointer.  This helps prevent nasty memory leak / double
deletion bugs typically resulting from programmers' omission to deal with such
issues in their destructors.

The @c -Weffc++ option in gcc generates warnings for noncompliance with Scott's
style guidelines, and hence necessitates the definition of this destructor,
trivial as it may be.
*/


