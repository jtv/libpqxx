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
 * Copyright (c) 2001-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifdef PQXX_HAVE_IOS
#include <ios>
#endif

#include <stdexcept>

#include "pqxx/util"

/* Methods tested in eg. self-test program test001 are marked with "//[t1]"
 */

// TODO: Support SQL arrays
// TODO: value_type, reference, const_reference
// TODO: container comparisons

namespace pqxx
{

/// Query or command result set.
/** This behaves as a container (as defined by the C++ standard library) and 
 * provides random access const iterators to iterate over its tuples.  A tuple 
 * can also be accessed by indexing a result R by the tuple's zero-based 
 * number:
 *
 *	for (result::size_type i=0; i < R.size(); ++i) Process(R[i]);
 */
class PQXX_LIBEXPORT result
{
public:
  result() throw () : m_Result(0), m_l(this), m_r(this) {}		//[t3]
  result(const result &rhs) throw () :					//[t1]
	  m_Result(0), m_l(this), m_r(this) { MakeRef(rhs); }
  ~result() { LoseRef(); }						//[t1]
  
  result &operator=(const result &) throw ();				//[t10]

  typedef unsigned long size_type;
  typedef signed long difference_type;
  class field;
  class const_fielditerator;

  /// Reference to one row in a result.
  /** A tuple represents one row (also called a tuple) in a query result set.  
   * It also acts as a container mapping column numbers or names to field 
   * values (see below):
   *
   * 	cout << tuple["date"].c_str() << ": " << tuple["name"].c_str() << endl;
   *
   * The tuple itself acts like a (non-modifyable) container, complete with its
   * own const_iterator and, if your compiler supports them,
   * const_reverse_iterator.
   */
  class PQXX_LIBEXPORT tuple
  {
  public:
    typedef unsigned int size_type;
    typedef signed int difference_type;
    typedef const_fielditerator const_iterator;

    tuple(const result *r, result::size_type i) throw () : 
      m_Home(r), m_Index(i) {}
    ~tuple() throw () {} // Yes Scott Meyers, you're absolutely right[1]

    inline const_iterator begin() const throw ();			//[t82]
    inline const_iterator end() const throw ();				//[t82]

#ifdef PQXX_HAVE_REVERSE_ITERATOR
    typedef PGSTD::reverse_iterator<const_iterator> const_reverse_iterator;
    const_reverse_iterator rbegin() const 				//[t82]
  	{ return const_reverse_iterator(end()); }
    const_reverse_iterator rend() const					//[t82]
   	{ return const_reverse_iterator(begin()); }
#endif

    inline field operator[](size_type) const throw ();			//[]
    inline field operator[](int i) const throw ()			//[]
    	{ return operator[](size_type(i)); }
    field operator[](const char[]) const;				//[t11]
    field operator[](const PGSTD::string &s) const 			//[t11]
    	{ return operator[](s.c_str()); }
    field at(size_type) const throw (PGSTD::out_of_range);		//[]
    field at(int i) const throw (PGSTD::out_of_range)			//[]
    	{ return at(size_type(i)); }
    field at(const char[]) const;					//[t11]
    field at(const PGSTD::string &s) const { return at(s.c_str()); }	//[t11]

    inline size_type size() const throw ();				//[t11]

    result::size_type rownumber() const throw () { return m_Index; }	//[t11]

    /// Number of given column (throws exception if it doesn't exist)
    size_type column_number(const PGSTD::string &ColName) const		//[t30]
    	{ return m_Home->column_number(ColName); }

    /// Number of given column (throws exception if it doesn't exist)
    size_type column_number(const char ColName[]) const			//[t30]
      	{ return m_Home->column_number(ColName); }

    /// Type of given column
    oid column_type(size_type ColNum) const				//[]
	{ return m_Home->column_type(ColNum); }

    /// Type of given column
    oid column_type(int ColNum) const					//[]
	{ return column_type(size_type(ColNum)); }

    /// Type of given column
    oid column_type(const PGSTD::string &ColName) const			//[t7]
      	{ return column_type(column_number(ColName)); }

    /// Type of given column
    oid column_type(const char ColName[]) const				//[t7]
      	{ return column_type(column_number(ColName)); }

    result::size_type num() const { return rownumber(); }		//[t1]

#ifdef PQXX_HAVE_PQFTABLE
    oid column_table(size_type ColNum) const				//[]
    	{ return m_Home->column_table(ColNum); }
    oid column_table(int ColNum) const					//[]
    	{ return column_table(size_type(ColNum)); }
    oid column_table(const PGSTD::string &ColName) const		//[t2]
      	{ return column_table(column_number(ColName)); }
#endif


#ifdef PQXX_DEPRECATED_HEADERS
    /// @deprecated Use rownumber() instead
    result::size_type Row() const { return rownumber(); }

    /// @deprecated Use column_number() instead
    size_type ColumnNumber(const PGSTD::string &ColName) const 
    	{ return m_Home->ColumnNumber(ColName); }

    /// @deprecated Use column_number() instead
    size_type ColumnNumber(const char ColName[]) const 
    	{ return m_Home->ColumnNumber(ColName); }
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
     * @param T tuple that this field is part of.
     * @param C column number of this field.
     */
    field(const tuple &T, tuple::size_type C) throw () : 		//[t1]
    	m_tup(T), m_col(C) {}

    /// Read as plain C string
    /** Since the field's data is stored internally in the form of a 
     * zero-terminated C string, this is the fastest way to read it.  Use the
     * to() or as() functions to convert the string to other types such as int,
     * or to C++ strings.
     */
    const char *c_str() const {return home()->GetValue(idx(),col());}	//[t2]

    /// Column name
    inline const char *name() const;					//[t11]

    /// Column type
    oid type() const 							//[t7]
    	{ return home()->column_type(col()); }

#ifdef PQXX_HAVE_PQFTABLE
    /// Table this field came from, if any
    /** Requires PostgreSQL 7.4 or greater
     */
    oid table() const { return home()->column_table(col()); }		//[t2]
#endif

    /// Read value into Obj; or leave Obj untouched and return false if null
    /** @warning The conversion is done using the currently active locale, 
     * whereas PostgreSQL delivers values in the "default" (C) locale.  This 
     * means that if you intend to use this function from a locale that doesn't
     * understand the data types in question (particularly numeric types like 
     * float and int) in default C format, you'll need to switch back to the C 
     * locale before the call--at least insofar as numeric formatting is
     * concerned (on POSIX systems, use setlocale(LC_NUMERIC, "C")).
     * This should be fixed at some point in the future.
     */
    template<typename T> bool to(T &Obj) const				//[t3]
    {
      if (is_null())
	return false;

      try
      {
        from_string(c_str(), Obj);
      }
      catch (const PGSTD::exception &e)
      {
	throw PGSTD::domain_error("Error reading field " + 
			          PGSTD::string(name()) +
				  ": " +
				  e.what());
      }
      return true;
    }

    /// Read value into Obj; or leave Obj untouched and return false if null
    template<typename T> bool operator>>(T &Obj) const			//[t7]
    	{ return to(Obj); }

#ifdef PQXX_NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION
    /// Specialization: to(string &)
    template<> bool to<PGSTD::string>(PGSTD::string &Obj) const;

    /// Specialization: to(const char *&).  
    /** The buffer has the same lifetime as the result, so take care not to
     * use it after the result is destroyed.
     */
    template<> bool to<const char *>(const char *&Obj) const;
#endif


    /// Read value into Obj; or use Default & return false if null
    template<typename T> bool to(T &Obj, const T &Default) const	//[t12]
    {
      const bool NotNull = to(Obj);
      if (!NotNull)
	Obj = Default;
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

    bool is_null() const { return home()->GetIsNull(idx(),col()); }	//[t12]

    size_type size() const { return home()->GetLength(idx(),col()); }	//[t11]

    tuple::size_type num() const { return col(); }			//[t82]

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

  /// Iterator for rows (tuples) in a query result set.
  /** A result, once obtained, cannot be modified.  Therefore there is no
   * plain iterator type for result.  However its const_iterator type can be 
   * used to inspect its tuples without changing them.
   */
  class PQXX_LIBEXPORT const_iterator : 
    public PGSTD::iterator<PGSTD::random_access_iterator_tag, 
                         const tuple,
                         result::size_type>, 
    public tuple
  {
  public:
    typedef result::size_type size_type;
    typedef result::difference_type difference_type;

    const_iterator() throw () : tuple(0,0) {}
    const_iterator(const tuple &t) throw () : tuple(t) {}

    /** The iterator "points to" its own tuple, which is also itself.  This 
     * allows a result to be addressed as a two-dimensional container without 
     * going through the intermediate step of dereferencing the iterator.  I 
     * hope this works out to be similar to C pointer/array semantics in useful 
     * cases[2].
     */
    pointer operator->()  const { return this; }			//[t12]
    reference operator*() const { return *operator->(); }		//[t12]

    const_iterator operator++(int);					//[t12]
    const_iterator &operator++() { ++m_Index; return *this; }		//[t1]
    const_iterator operator--(int);					//[t12]
    const_iterator &operator--() { --m_Index; return *this; }		//[t12]

    const_iterator &operator+=(difference_type i) 			//[t12]
    	{ m_Index+=i; return *this; }
    const_iterator &operator-=(difference_type i) 			//[t12]
    	{ m_Index-=i; return *this; }

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

    inline const_iterator operator+(difference_type) const;		//[t12]

    friend const_iterator operator+(difference_type, const_iterator);	//[t12]

    inline const_iterator operator-(difference_type) const;		//[t12]

    inline difference_type operator-(const_iterator) const;		//[t12]

  private:
    friend class result;
    const_iterator(const result *r, result::size_type i) : tuple(r, i) {}
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
    typedef tuple::size_type size_type;
    typedef tuple::difference_type difference_type;
    using it::pointer;
    using it::reference;

    const_fielditerator(const tuple &T, tuple::size_type C) throw () :	//[t82]
      field(T, C) {}
    const_fielditerator(const field &F) throw () : field(F) {}		//[t82]

    pointer operator->() const { return this; }				//[t82]
    reference operator*() const { return *operator->(); }		//[t82]

    const_fielditerator operator++(int);				//[t82]
    const_fielditerator &operator++() { ++m_col; return *this; }	//[t82]
    const_fielditerator operator--(int);				//[t82]
    const_fielditerator &operator--() { --m_col; return *this; }	//[t82]

    const_fielditerator &operator+=(difference_type i) 			//[t82]
    	{ m_col+=i; return *this; }
    const_fielditerator &operator-=(difference_type i) 			//[t82]
    	{ m_col-=i; return *this; }

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

    inline const_fielditerator operator+(difference_type) const;	//[t82]

    friend const_fielditerator operator+(difference_type, 
		    		    const_fielditerator);		//[t82]

    inline const_fielditerator operator-(difference_type) const;	//[t82]

    inline difference_type operator-(const_fielditerator) const;	//[t82]
  };


#ifdef PQXX_HAVE_REVERSE_ITERATOR
  typedef PGSTD::reverse_iterator<const_iterator> const_reverse_iterator;
  const_reverse_iterator rbegin() const 				//[t75]
  	{ return const_reverse_iterator(end()); }
  const_reverse_iterator rend() const					//[t75]
   	{ return const_reverse_iterator(begin()); }
#endif

  const_iterator begin() const { return const_iterator(this, 0); }	//[t1]
  inline const_iterator end() const;					//[t1]

  size_type size() const						//[t2]
  	{ return m_Result ? PQXXPQ::PQntuples(m_Result) : 0; }
  bool empty() const							//[t11]
  	{ return !m_Result || !PQXXPQ::PQntuples(m_Result); }
  size_type capacity() const { return size(); }				//[t20]

  void swap(result &other) throw ();					//[t77]

  const tuple operator[](size_type i) const throw () 			//[t2]
  	{ return tuple(this, i); }
  const tuple at(size_type) const throw (PGSTD::out_of_range);		//[t10]

  void clear() throw () { LoseRef(); }					//[t20]

  /// Number of columns in result
  tuple::size_type columns() const throw () 				//[t11]
  	{ return PQnfields(m_Result); }

  /// Number of given column (throws exception if it doesn't exist)
  tuple::size_type column_number(const char ColName[]) const;		//[t11]

  /// Number of given column (throws exception if it doesn't exist)
  tuple::size_type column_number(const PGSTD::string &Name) const	//[t11]
  	{return column_number(Name.c_str());}

  /// Name of column with this number (throws exception if it doesn't exist)
  const char *column_name(tuple::size_type Number) const;		//[t11]

  /// Type of given column
  inline oid column_type(tuple::size_type ColNum) const;		//[]
  /// Type of given column
  inline oid column_type(int ColNum) const				//[]
  	{ return column_type(tuple::size_type(ColNum)); }

  /// Type of given column
  oid column_type(const PGSTD::string &ColName) const			//[t7]
    	{ return column_type(column_number(ColName)); }

  /// Type of given column
  oid column_type(const char ColName[]) const				//[t7]
    	{ return column_type(column_number(ColName)); }

#ifdef PQXX_HAVE_PQFTABLE
  /// Table that given column was taken from, if any
  oid column_table(tuple::size_type ColNum) const;			//[]
  /// Table that given column was taken from, if any
  oid column_table(int ColNum) const					//[]
  	{ return column_table(tuple::size_type(ColNum)); } 

  /// Table that given column was taken from, if any
  oid column_table(const PGSTD::string &ColName) const			//[t2]
    	{ return column_table(column_number(ColName)); }
#endif

  /// If command was INSERT of 1 row, return oid of inserted row
  /** Returns oid_none otherwise. 
   */
  oid inserted_oid() const { return PQoidValue(m_Result); }		//[t13]


  /// If command was INSERT, UPDATE, or DELETE, return number of affected rows
  /*** Returns zero for all other commands. */
  size_type affected_rows() const;					//[t7]


#ifdef PQXX_DEPRECATED_HEADERS
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
  	{return PQfnumber(m_Result,Name);}
  /// @deprecated Use column_number() instead
  tuple::size_type ColumnNumber(const PGSTD::string &Name) const
  	{return ColumnNumber(Name.c_str());}
  /// @deprecated Use column_name() instead
  const char *ColumnName(tuple::size_type Number) const
  	{return PQfname(m_Result,Number);}
#endif


private:
  internal::pq::PGresult *m_Result;
  mutable const result *m_l, *m_r;

  friend class result::field;
  const char *GetValue(size_type Row, tuple::size_type Col) const;
  bool GetIsNull(size_type Row, tuple::size_type Col) const;
  field::size_type GetLength(size_type Row, tuple::size_type Col) const;

  friend class connection_base;
  friend class pipeline;
  explicit result(internal::pq::PGresult *rhs) throw () : 
    m_Result(0), m_l(this), m_r(this) {MakeRef(rhs);}
  result &operator=(internal::pq::PGresult *) throw ();
  bool operator!() const throw () { return !m_Result; }
  operator bool() const throw () { return m_Result != 0; }
  void CheckStatus(const PGSTD::string &Query) const;
  void CheckStatus(const char Query[]) const;
  int errorposition() const throw ();
  PGSTD::string StatusError() const;

  friend class Cursor;
  const char *CmdStatus() const throw () { return PQcmdStatus(m_Result); }


  void MakeRef(internal::pq::PGresult *) throw ();
  void MakeRef(const result &) throw ();
  void LoseRef() throw ();
};


/// Write a result field to any type of stream
/** This can be convenient when writing a field to an output stream.  More
 * importantly, it lets you write a field to e.g. a stringstream which you can
 * then use to read, format and convert the field in ways that to() does not
 * support.  
 *
 * Example: parse a field into a variable of the nonstandard "long long" type.
 *
 * extern result R;
 * long long L;
 * stringstream S;
 *
 * // Write field's string into S
 * S << R[0][0];
 *
 * // Parse contents of S into L
 * S >> L;
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



inline result::tuple::const_iterator
result::tuple::begin() const throw ()
	{ return tuple::const_iterator(*this, 0); }

inline result::tuple::const_iterator
result::tuple::end() const throw ()
	{ return tuple::const_iterator(*this, size()); }

inline result::field 
result::tuple::operator[](result::tuple::size_type i) const  throw ()
	{ return field(*this, i); }

inline result::tuple::size_type result::tuple::size() const throw ()
	{ return m_Home->columns(); }

inline const char *result::field::name() const 
	{ return home()->column_name(col()); }

/// Specialization: to(string &)
template<> 
inline bool result::field::to<PGSTD::string>(PGSTD::string &Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}

/// Specialization: to(const char *&).  
/** The buffer has the same lifetime as the result, so take care not to
 * use it after the result is destroyed.
 */
template<> 
inline bool result::field::to<const char *>(const char *&Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}


inline result::const_iterator 
result::const_iterator::operator+(difference_type o) const
{
  return const_iterator(m_Home, m_Index + o);
}

inline result::const_iterator 
operator+(result::const_iterator::difference_type o, 
	  result::const_iterator i)
{
  return i + o;
}

inline result::const_iterator 
result::const_iterator::operator-(difference_type o) const
{
  return const_iterator(m_Home, m_Index - o);
}

inline result::const_iterator::difference_type 
result::const_iterator::operator-(const_iterator i) const
{ 
  return num()-i.num(); 
}

inline result::const_iterator result::end() const 
{ 
  return const_iterator(this, size()); 
}

inline oid result::column_type(tuple::size_type ColNum) const
{
  const oid T = PQftype(m_Result, ColNum);
  if (T == oid_none)
    throw PGSTD::invalid_argument(
		"Attempt to retrieve type of nonexistant column " +
	        to_string(ColNum) + " "
		"of query result");
  return T;
}


inline result::const_fielditerator 
result::const_fielditerator::operator+(difference_type o) const
{
  return const_fielditerator(m_tup, col() + o);
}

inline result::const_fielditerator 
operator+(result::const_fielditerator::difference_type o,
	  result::const_fielditerator i)
{
  return i + o;
}

inline result::const_fielditerator 
result::const_fielditerator::operator-(difference_type o) const
{
  return const_fielditerator(m_tup, col() - o);
}

inline result::const_fielditerator::difference_type 
result::const_fielditerator::operator-(const_fielditerator i) const
{ 
  return num()-i.num(); 
}



#ifdef PQXX_HAVE_PQFTABLE
inline oid result::column_table(tuple::size_type ColNum) const
{
  const oid T = PQftable(m_Result, ColNum);

  /* If we get oid_none, it may be because the column is computed, or because
   * we got an invalid row number.
   */
  // TODO: Skip this if we first computed the column name ourselves
  if ((T == oid_none) &&
      (ColNum >= columns()))
    throw PGSTD::invalid_argument("Attempt to retrieve table ID for column " +
				  to_string(ColNum) + " "
				  "out of " + to_string(columns()));
  return T;
}
#endif


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
  {
    return traits_type::eof();
  }

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
 * field.  All formatting and streaming operations of std::istream are 
 * supported.  What you'll typically want to use, however, is the fieldstream
 * typedef (which defines a basic_fieldstream for char).  This is similar to how
 * e.g. std::ifstream relates to std::basic_ifstream.
 *
 * When reading bools, ints, floats etc. from a field, bear in mind that bools
 * are represented as 'f' or 't' (for false or true, respectively), and that all
 * other non-string types are rendered in the default C locale.  If there is any
 * chance of your program running in a different locale, you'll need to set the
 * locale to "C" explicitly while parsing fields.
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

  basic_fieldstream(const result::field &F) : super(&m_Buf), m_Buf(F) { }

private:
  field_streambuf<CHAR, TRAITS> m_Buf;
};

typedef basic_fieldstream<char> fieldstream;

} // namespace pqxx



/* 
[1] Scott Meyers, in one of his essential books, "Effective C++" and "More 
Effective C++", points out that it is good style to have any class containing 
a member of pointer type define its own destructor--just to show that it knows
what it is doing.  This helps prevent nasty memory leak / double deletion bugs
typically resulting from programmers' omission to deal with such issues in
their destructors.

The -Weffc++ option in gcc generates warnings for noncompliance with Scott's
style guidelines, and hence necessitates the definition of this destructor,\
trivial as it may be.

[2] IIRC Alex Stepanov, the inventor of the STL, once remarked that having
this as standard behaviour for pointers would be useful in some algorithms.
So even if this makes me look foolish, I would seem to be in distinguished 
company.
*/


