/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/result.h
 *
 *   DESCRIPTION
 *      definitions for the pqxx::Result class and support classes.
 *   pqxx::Result represents the set of result tuples from a database query
 *
 * Copyright (c) 2001-2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_RESULT_H
#define PQXX_RESULT_H

#include <stdexcept>

#include "pqxx/util.h"

/* Methods tested in eg. self-test program test1 are marked with "//[t1]"
 */


// TODO: Support postgres arrays

namespace pqxx
{

/// Query or command result set.
/** This behaves as a container (as defined by the C++ standard library) and 
 * provides random access const iterators to iterate over its tuples.  A tuple 
 * can also be accessed by indexing a Result R by the tuple's zero-based 
 * number:
 *
 *	for (Result::size_type i=0; i < R.size(); ++i) Process(R[i]);
 */
class PQXX_LIBEXPORT Result
{
public:
  Result() : m_Result(0), m_Refcount(0) {}				//[t3]
  Result(const Result &rhs) : 						//[t1]
	  m_Result(0), m_Refcount(0) { MakeRef(rhs); }
  ~Result() { LoseRef(); }						//[t1]
  
  Result &operator=(const Result &);					//[t10]

  typedef Result_size_type size_type;
  class Field;

  // TODO: Field iterators
 
  /// Reference to one row in a Result.
  /** A Tuple represents one row (also called a tuple) in a query result set.  
   * It also acts as a container mapping column numbers or names to Field 
   * values (see below):
   *
   * 	cout << Tuple["date"].c_str() << ": " << Tuple["name"].c_str() << endl;
   *
   * The fields in a Tuple can not currently be iterated over.
   */
  class PQXX_LIBEXPORT Tuple
  {
  public:
    typedef Tuple_size_type size_type;
    Tuple(const Result *r, Result::size_type i) : m_Home(r), m_Index(i) {}
    ~Tuple() {} // Yes Scott Meyers, you're absolutely right[1]

    inline Field operator[](size_type) const;				//[t1]
    Field operator[](const char[]) const;				//[t11]
    Field operator[](const PGSTD::string &s) const 			//[t11]
    	{ return operator[](s.c_str()); }
    Field at(size_type) const;						//[t10]
    Field at(const char[]) const;					//[t11]
    Field at(const PGSTD::string &s) const { return at(s.c_str()); }	//[t11]

    inline size_type size() const;					//[t11]

    Result::size_type Row() const { return m_Index; }			//[t11]

    size_type ColumnNumber(const char ColName[]) const 			//[]
    	{ return m_Home->ColumnNumber(ColName); }

  protected:
    const Result *m_Home;
    Result::size_type m_Index;

    // Not allowed:
    Tuple();
  };


  /// Reference to a field in a result set.
  /** A Field represents one entry in a Tuple.  It represents an actual value 
   * in the result set, and can be converted to various types.
   */
  class PQXX_LIBEXPORT Field : private Tuple
  {
  public:
    typedef size_t size_type;

    /// Constructor.
    /** Create Field as reference to a field in a result set.
     * @param R Tuple that this Field is part of.
     * @param C column number of this field.
     */
    Field(const Tuple &R, Tuple::size_type C) : Tuple(R), m_Col(C) {}	//[t1]

    /// Read as plain C string
    /** Since the field's data is stored internally in the form of a 
     * zero-terminated C string, this is the fastest way to read it.  Use the
     * to() functions to convert the string to other types such as int, or to
     * C++ strings.
     */
    const char *c_str() const {return m_Home->GetValue(m_Index,m_Col);}	//[t2]

    /// Column name
    inline const char *Name() const;					//[t11]

    /// Read value into Obj; or leave Obj untouched & return false if null
    template<typename T> bool to(T &Obj) const				//[t1]
    {
      if (is_null())
	return false;

      try
      {
        FromString(c_str(), Obj);
      }
      catch (const PGSTD::exception &e)
      {
	throw PGSTD::runtime_error("Error reading field " + 
			           PGSTD::string(Name()) +
				   ": " +
				   e.what());
      }
      return true;
    }


#ifdef NO_PARTIAL_CLASS_TEMPLATE_SPECIALISATION
    /// Specialization: to(string &)
    template<> bool to<PGSTD::string>(PGSTD::string &Obj) const;

    /// Specialization: to(const char *&).  
    /** The buffer has the same lifetime as the Result, so take care not to
     * use it after the Result is destroyed.
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

    bool is_null() const { return m_Home->GetIsNull(m_Index,m_Col); }	//[t12]

    int size() const { return m_Home->GetLength(m_Index,m_Col); }	//[t11]

  private:

    Tuple::size_type m_Col;
  };


  /// Iterator for rows (tuples) in a query result set.
  /** A Result, once obtained, cannot be modified.  Therefore there is no
   * plain iterator type for Result.  However its const_iterator type can be 
   * used to inspect its Tuples without changing them.
   */
  class PQXX_LIBEXPORT const_iterator : 
    public PGSTD::iterator<PGSTD::random_access_iterator_tag, 
                         const Tuple,
                         Result::size_type>, 
    public Tuple
  {
  public:
    const_iterator() : Tuple(0,0) {}

    /** The iterator "points to" its own Tuple, which is also itself.  This 
     * allows a Result to be addressed as a two-dimensional container without 
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

    inline const_iterator operator+(difference_type o) const;		//[t12]

    friend const_iterator operator+(difference_type o, 
		    		    const_iterator i);			//[t12]

    inline const_iterator operator-(difference_type o) const;		//[t12]

    inline difference_type operator-(const_iterator i) const;		//[t12]

    Result::size_type num() const { return Row(); }			//[t1]

  private:
    friend class Result;
    const_iterator(const Result *r, Result::size_type i) : Tuple(r, i) {}
  };

  const_iterator begin() const { return const_iterator(this, 0); }	//[t1]
  inline const_iterator end() const;					//[t1]
  // TODO: Reverse iterators

  size_type size() const { return m_Result ? PQntuples(m_Result) : 0; }	//[t2]
  bool empty() const { return !m_Result || !PQntuples(m_Result); }	//[t11]
  size_type capacity() const { return size(); }				//[t20]

  const Tuple operator[](size_type i) const { return Tuple(this, i); }	//[t2]
  const Tuple at(size_type) const;					//[t10]

  void clear() { LoseRef(); }						//[t20]

  Tuple::size_type Columns() const { return PQnfields(m_Result); }	//[t11]

  /// Number of given column, or -1 if it does not exist
  Tuple::size_type ColumnNumber(const char Name[]) const 		//[t11]
  	{return PQfnumber(m_Result,Name);}
  /// Number of given column, or -1 if it does not exist
  Tuple::size_type ColumnNumber(const std::string &Name) const 		//[t11]
  	{return ColumnNumber(Name.c_str());}
  const char *ColumnName(Tuple::size_type Number) const 		//[t11]
  	{return PQfname(m_Result,Number);}

  /// If command was INSERT of 1 row, return oid of inserted row
  /** Returns InvalidOid otherwise. */
  Oid InsertedOid() const { return PQoidValue(m_Result); }		//[t13]

  /// If command was INSERT, UPDATE, or DELETE, return number of affected rows
  /*** Returns zero for all other commands. */
  size_type AffectedRows() const;					//[t7]

private:
  PGresult *m_Result;
  mutable int *m_Refcount;

  friend class Result::Field;
  const char *GetValue(size_type Row, Tuple::size_type Col) const;
  bool GetIsNull(size_type Row, Tuple::size_type Col) const;
  Field::size_type GetLength(size_type Row, Tuple::size_type Col) const;

  friend class ConnectionItf;
  explicit Result(PGresult *rhs) : m_Result(rhs), m_Refcount(0) {MakeRef(rhs);}
  Result &operator=(PGresult *);
  bool operator!() const throw () { return !m_Result; }
  operator bool() const throw () { return m_Result != 0; }
  void CheckStatus(const PGSTD::string &Query) const;

  friend class Cursor;
  const char *CmdStatus() const throw () { return PQcmdStatus(m_Result); }


  void MakeRef(PGresult *);
  void MakeRef(const Result &);
  void LoseRef() throw ();
};


inline Result::Field 
Result::Tuple::operator[](Result::Tuple::size_type i) const 
{ 
  return Field(*this, i); 
}

inline Result::Tuple::size_type Result::Tuple::size() const 
{ 
  return m_Home->Columns(); 
}

inline const char *Result::Field::Name() const 
{ 
  return m_Home->ColumnName(m_Col); 
}

/// Specialization: to(string &)
template<> 
inline bool Result::Field::to<PGSTD::string>(PGSTD::string &Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}

/// Specialization: to(const char *&).  
/** The buffer has the same lifetime as the Result, so take care not to
 * use it after the Result is destroyed.
 */
template<> 
inline bool Result::Field::to<const char *>(const char *&Obj) const
{
  if (is_null()) return false;
  Obj = c_str();
  return true;
}


inline Result::const_iterator 
Result::const_iterator::operator+(difference_type o) const
{
  return const_iterator(m_Home, m_Index + o);
}

inline Result::const_iterator 
operator+(Result::const_iterator::difference_type o, 
	  Result::const_iterator i)
{
  return i + o;
}

inline Result::const_iterator 
Result::const_iterator::operator-(difference_type o) const
{
  return const_iterator(m_Home, m_Index - o);
}

inline Result::const_iterator::difference_type 
Result::const_iterator::operator-(const_iterator i) const
{ 
  return num()-i.num(); 
}

inline Result::const_iterator Result::end() const 
{ 
  return const_iterator(this, size()); 
}

} // namespace pqxx


/// Write a result field to any type of stream
/** This can be convenient when writing a Field to an output stream.  More
 * importantly, it lets you write a Field to e.g. a stringstream which you can
 * then use to read, format and convert the field in ways that to() does not
 * support.  
 *
 * Example: parse a Field into a variable of the nonstandard "long long" type.
 *
 * extern Result R;
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
inline STREAM &operator<<(STREAM &S, const pqxx::Result::Field &F)	//[t46]
{
  S << F.c_str();
  return S;
}



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

#endif

