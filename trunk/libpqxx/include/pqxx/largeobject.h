/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/largeobject.h
 *
 *   DESCRIPTION
 *      libpqxx's Large Objects interface
 *   Allows access to large objects directly, or through I/O streams
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_LARGEOBJECT_H
#define PQXX_LARGEOBJECT_H

#include <streambuf>

#include <pqxx/transactionitf.h>


namespace pqxx
{

class LargeObjectAccess;


/// Identity of a large object
/** This class encapsulates the identity of a large object.  To access the
 * contents of the object, create a LargeObjectAccess, a largeobject_streambuf,
 * or an ilostream, an olostream or a lostream around the LargeObject.
 *
 * A LargeObject must be accessed only from within a backend transaction, but
 * the object's identity remains valid.
 */
class LargeObject
{
public:
  typedef long size_type;

  /// Refer to a nonexistent large object (similar to what a null pointer does)
  LargeObject();							//[t48]

  /// Create new large object
  /** @param T backend transaction in which the object is to be created
   */
  explicit LargeObject(TransactionItf &T);				//[t48]

  /// Wrap object with given Oid
  /** Convert combination of a transaction and object identifier into a
   * large object identity.  Does not affect the database.
   * @param O object identifier for the given object
   */
  explicit LargeObject(Oid O) : m_ID(O) {}				//[t48]

  /// Import large object from a local file
  /** Creates a large object containing the data found in the given file.
   * @param T the backend transaction in which the large object is to be created
   * @param File a filename on the client program's filesystem
   */
  LargeObject(TransactionItf &T, const PGSTD::string &File);		//[]

  /// Take identity of an opened large object
  /** Copy identity of already opened large object.  Note that this may be done
   * as an implicit conversion.
   * @param O already opened large object to copy identity from
   */
  LargeObject(const LargeObjectAccess &O);				//[t50]

  /// Object identifier
  /** The number returned by this function identifies the large object in the
   * database we're connected to.
   */
  Oid id() const throw () { return m_ID; }				//[t48]

  /// Comparison is only valid between large objects in the same database.
  bool operator==(const LargeObject &other) const 			//[t51]
	  { return m_ID == other.m_ID; }
  /// Comparison is only valid between large objects in the same database.
  bool operator!=(const LargeObject &other) const 			//[t51]
	  { return m_ID != other.m_ID; }
  /// Comparison is only valid between large objects in the same database.
  bool operator<=(const LargeObject &other) const 			//[t51]
	  { return m_ID <= other.m_ID; }
  /// Comparison is only valid between large objects in the same database.
  bool operator>=(const LargeObject &other) const 			//[t51]
	  { return m_ID >= other.m_ID; }
  /// Comparison is only valid between large objects in the same database.
  bool operator<(const LargeObject &other) const 			//[t51]
	  { return m_ID < other.m_ID; }
  /// Comparison is only valid between large objects in the same database.
  bool operator>(const LargeObject &other) const 			//[t51]
	  { return m_ID > other.m_ID; }

  /// Export large object's contents to a local file
  /** Writes the data stored in the large object to the given file.
   * @param T the transaction in which the object is to be accessed
   * @param File a filename on the client's filesystem
   */
  void to_file(TransactionItf &T, const char File[]) const;		//[]

  /// Export large object's contents to a local file
  /** Writes the data stored in the large object to the given file.
   * @param T the transaction in which the object is to be accessed
   * @param File a filename on the client's filesystem
   */
  void to_file(TransactionItf &T, const PGSTD::string &File) const 	//[]
  { 
    to_file(T, File.c_str()); 
  }

  /// Delete large object from database
  /** As opposed to its low-level equivalent cunlink, this will throw an
   * exception if deletion fails.
   * @param T the transaction in which the object is to be deleted
   */
  void remove(TransactionItf &T) const;					//[t48]

protected:
  static PGconn *RawConnection(const TransactionItf &T)
  {
    return T.Conn().RawConnection();
  }

  PGSTD::string Reason() const;

private:
  Oid m_ID;
};


/// Accessor for large object's contents.
class LargeObjectAccess : private LargeObject
{
public:
  using LargeObject::size_type;

  /// Create new large object and open it
  /** 
   * @param T backend transaction in which the object is to be created
   * @param mode access mode, defaults to ios_base::in | ios_base::out
   */
  explicit LargeObjectAccess(TransactionItf &T, 
			     PGSTD::ios_base::openmode mode = 
				PGSTD::ios_base::in | 
				PGSTD::ios_base::out);			//[t51]

  /// Open large object with given Oid
  /** Convert combination of a transaction and object identifier into a
   * large object identity.  Does not affect the database.
   * @param T the transaction in which the object is to be accessed
   * @param O object identifier for the given object
   * @param mode access mode, defaults to ios_base::in | ios_base::out
   */
  explicit LargeObjectAccess(TransactionItf &T, 
			     Oid O,
			     PGSTD::ios_base::openmode mode = 
				PGSTD::ios_base::in | 
				PGSTD::ios_base::out);			//[]

  /// Open given large object
  /** Open a large object with the given identity for reading and/or writing
   * @param T the transaction in which the object is to be accessed
   * @param O identity for the large object to be accessed
   * @param mode access mode, defaults to ios_base::in | ios_base::out
   */
  LargeObjectAccess(TransactionItf &T, 
		    LargeObject O,
		    PGSTD::ios_base::openmode mode = 
				PGSTD::ios_base::in | 
				PGSTD::ios_base::out);			//[t50]

  /// Import large object from a local file and open it
  /** Creates a large object containing the data found in the given file.
   * @param T the backend transaction in which the large object is to be created
   * @param File a filename on the client program's filesystem
   * @param mode access mode, defaults to ios_base::in | ios_base::out
   */
  LargeObjectAccess(TransactionItf &T, 
		    const PGSTD::string &File,
		    PGSTD::ios_base::openmode mode = 
			PGSTD::ios_base::in | PGSTD::ios_base::out);	//[]

  ~LargeObjectAccess() { close(); }

  /// Object identifier
  /** The number returned by this function identifies the large object in the
   * database we're connected to.
   */
  using LargeObject::id;

  /// Export large object's contents to a local file
  /** Writes the data stored in the large object to the given file.
   * @param File a filename on the client's filesystem
   */
  void to_file(const char File[]) const 				//[]
  { 
    LargeObject::to_file(m_Trans, File); 
  }

  /// Export large object's contents to a local file
  /** Writes the data stored in the large object to the given file.
   * @param File a filename on the client's filesystem
   */
  void to_file(const PGSTD::string &File) const 			//[]
  { 
    LargeObject::to_file(m_Trans, File); 
  }

  /// Write data to large object
  /** If not all bytes could be written, an exception is thrown.
   * @param Buf the data to write
   * @param Len the number of bytes from Buf to write
   */
  void write(const char Buf[], size_type Len);				//[]

  /// Write string to large object
  /** If not all bytes could be written, an exception is thrown.
   * @param Buf the data to write; no terminating zero is written
   */
  void write(const PGSTD::string &Buf)					//[t50]
  	{ write(Buf.c_str(), Buf.size()); }

  /// Read data from large object
  /** Returns the number of bytes read, which may be less than the number of
   * bytes requested if the end of the large object is reached.  Throws an
   * exception if an error occurs while reading.
   * @param Buf the location to store the read data in
   * @param Len the number of bytes to try and read
   */
  size_type read(char Buf[], size_type Len);				//[t50]

  /// Seek in large object's data stream
  /** Returns the new position in the large object.  Throws an exception if an
   * error occurs.
   */
  size_type seek(size_type dest, PGSTD::ios_base::seekdir dir);		//[t51]

  /// Seek in large object's data stream
  /** Does not throw exception in case of error; inspect return value and errno
   * instead.
   * Returns new position in large object, or -1 if an error occurred.
   * @param dest offset to go to
   * @param dir origin to which dest is relative: ios_base::beg (from beginning 
   *        of the object), ios_base::cur (from current access position), or
   *        ios_base;:end (from end of object)
   */
  long cseek(long dest, PGSTD::ios_base::seekdir dir) throw ();		//[t50]
    
  /// Write to large object's data stream
  /** Does not throw exception in case of error; inspect return value and errno
   * instead.
   * Returns number of bytes actually written, or -1 if an error occurred.
   * @param Buf bytes to write
   * @param Len number of bytes to write
   */
  long cwrite(const char Buf[], size_type Len) throw ();		//[t50]

  /// Read from large object's data stream
  /** Does not throw exception in case of error; inspect return value and errno
   * instead.
   * Returns number of bytes actually read, or -1 if an error occurred.
   * @param Buf area where bytes should be stored
   * @param Len number of bytes to read
   */
  long cread(char Buf[], size_type Len) throw ();			//[t50]

  using LargeObject::operator==;
  using LargeObject::operator!=;
  using LargeObject::operator<;
  using LargeObject::operator<=;
  using LargeObject::operator>;
  using LargeObject::operator>=;

private:
  PGSTD::string Reason() const;
  PGconn *RawConnection() { return LargeObject::RawConnection(m_Trans); }

  void open(PGSTD::ios_base::openmode mode);
  void close();

  TransactionItf &m_Trans;
  int m_fd;

  // Not allowed:
  LargeObjectAccess();
  LargeObjectAccess(const LargeObjectAccess &);
  LargeObjectAccess operator=(const LargeObjectAccess &);
};


/// Streambuf to use large objects in standard I/O streams
/** The standard streambuf classes provide uniform access to data storage such
 * as files or string buffers, so they can be accessed using standard input or
 * output streams.  This streambuf implementation provides similar access to
 * large objects, so they can be read and written using the same stream classes.
 */
template<typename CHAR=char, typename TRAITS=PGSTD::char_traits<CHAR> >
class largeobject_streambuf : public PGSTD::basic_streambuf<CHAR, TRAITS>
{
  typedef long size_type;
public:
  typedef CHAR   char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;

  largeobject_streambuf(TransactionItf &T,
			LargeObject O,
			PGSTD::ios_base::openmode mode = 
				PGSTD::ios::in | PGSTD::ios::out,
			size_type BufSize=512) : 			//[t48]
    m_BufSize(BufSize),
    m_Obj(T, O),
    m_G(0),
    m_P(0)
  {
    initialize(mode);
  }

  largeobject_streambuf(TransactionItf &T,
			Oid O,
			PGSTD::ios_base::openmode mode = 
				PGSTD::ios::in | PGSTD::ios::out,
			size_type BufSize=512) : 			//[t48]
    m_BufSize(BufSize),
    m_Obj(T, O),
    m_G(0),
    m_P(0)
  {
    initialize(mode);
  }

  virtual ~largeobject_streambuf()
  {
    delete [] m_P;
    delete [] m_G;
  }


protected:
  virtual int sync()
  {
    // setg() sets eback, gptr, egptr
    setg(eback(), eback(), egptr());
    return overflow(EoF());
  }

  virtual pos_type seekoff(off_type offset, 
			   PGSTD::ios_base::seekdir dir,
			   PGSTD::ios_base::openmode mode =
				PGSTD::ios_base::in|PGSTD::ios_base::out)
  {
    if (!mode) {}	// Quench "unused parameter" warning
    return AdjustEOF(m_Obj.cseek(offset, dir));
  }

  virtual pos_type seekpos(pos_type pos, 
			   PGSTD::ios_base::openmode mode =
				PGSTD::ios_base::in|PGSTD::ios_base::out)
  {
    if (!mode) {}	// Quench "unused parameter" warning
    return AdjustEOF(m_Obj.cseek(pos, PGSTD::ios_base::beg));
  }

  virtual int_type overflow(int_type ch = EoF())
  {
    char *const pp = pptr();
    if (!pp) return EoF();
    char *const pb = pbase();
    int_type result = 0;

    if (pp > pb) result = AdjustEOF(m_Obj.cwrite(pb, pp-pb));
    setp(m_P, m_P + m_BufSize);

    // Write that one more character, if it's there.
    if (ch != EoF())
    {
      *pptr() = char(ch);
      pbump(1);
    }
    return result;
  }

  virtual int_type underflow()
  {
    if (!gptr()) return EoF();
    char *const eb = eback();
    const int result = AdjustEOF(m_Obj.cread(eback(), m_BufSize));
    setg(eb, eb, eb + ((result==EoF()) ? 0 : result));
    return (!result || (result == EoF())) ? EoF() : *eb;
  }

private:
  /// Shortcut for traits_type::eof()
  static int_type EoF() { return traits_type::eof(); }

  /// Helper: change error position of -1 to EOF (probably a no-op)
  static PGSTD::streampos AdjustEOF(int pos)
  {
    return (pos == -1) ? EoF() : pos;
  }

  void initialize(PGSTD::ios_base::openmode mode)
  {
    if (mode & PGSTD::ios_base::in) 
    {
      m_G = new char_type[m_BufSize];
      setg(m_G, m_G, m_G);
    }
    if (mode & PGSTD::ios_base::out)
    {
      m_P = new char_type[m_BufSize];
      setp(m_P, m_P + m_BufSize);
    }
  }

  const size_type m_BufSize;
  LargeObjectAccess m_Obj;

  // Get & put buffers
  char_type *m_G, *m_P;
};


/// Input stream that gets its data from a large object
/** Use this class exactly as you would any other istream to read data from a
 * large object.  All formatting and streaming operations of std::istream are 
 * supported.  What you'll typically want to use, however, is the ilostream
 * typedef defined below.  This is similar to e.g. std::ifstream.
 *
 * Currently only works for <char, std::char_traits<char> >.
 */
template<typename CHAR=char, typename TRAITS=PGSTD::char_traits<CHAR> > 
class basic_ilostream : public PGSTD::basic_istream<CHAR, TRAITS>
{
public:
  typedef CHAR char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;

  /// Create a basic_ilostream
  /** @param T transaction in which this stream is to exist
   * @param O a large object to access
   * @param BufSize size of buffer to use internally (optional)
   */
  basic_ilostream(TransactionItf &T, 
                  LargeObject O, 
		  LargeObject::size_type BufSize=512) :			//[]
    PGSTD::basic_istream<CHAR,TRAITS>(&m_Buf),
    m_Buf(T, O, in, BufSize) 
  { 
  }

  /// Create a basic_ilostream
  /** @param T transaction in which this stream is to exist
   * @param O identifier of a large object to access
   * @param BufSize size of buffer to use internally (optional)
   */
  basic_ilostream(TransactionItf &T, 
                  Oid O, 
		  LargeObject::size_type BufSize=512) :			//[t48]
    PGSTD::basic_istream<CHAR,TRAITS>(&m_Buf),
    m_Buf(T, O, in, BufSize) 
  { 
  }

private:
  largeobject_streambuf<CHAR,TRAITS> m_Buf;
};

typedef basic_ilostream<char> ilostream;


/// Output stream that writes data back to a large object
/** Use this class exactly as you would any other ostream to write data to a
 * large object.  All formatting and streaming operations of std::ostream are 
 * supported.  What you'll typically want to use, however, is the olostream
 * typedef defined below.  This is similar to e.g. std::ofstream.
 *
 * Currently only works for <char, std::char_traits<char> >.
 */
template<typename CHAR=char, typename TRAITS=PGSTD::char_traits<CHAR> > 
class basic_olostream : public PGSTD::basic_ostream<CHAR, TRAITS>
{
public:
  typedef CHAR char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;

  /// Create a basic_olostream
  /** @param T transaction in which this stream is to exist
   * @param O a large object to access
   * @param BufSize size of buffer to use internally (optional)
   */
  basic_olostream(TransactionItf &T, 
                  LargeObject O,
		  LargeObject::size_type BufSize=512) :			//[t48]
    PGSTD::basic_ostream<CHAR,TRAITS>(&m_Buf),
    m_Buf(T, O, out, BufSize) 
  { 
  }

  /// Create a basic_olostream
  /** @param T transaction in which this stream is to exist
   * @param O a large object to access
   * @param BufSize size of buffer to use internally (optional)
   */
  basic_olostream(TransactionItf &T, 
      		  Oid O,
		  LargeObject::size_type BufSize=512) :			//[]
    PGSTD::basic_ostream<CHAR,TRAITS>(&m_Buf),
    m_Buf(T, O, out, BufSize) 
  { 
  }

  ~basic_olostream() { m_Buf.pubsync(); m_Buf.pubsync(); }

private:
  largeobject_streambuf<CHAR,TRAITS> m_Buf;
};

typedef basic_olostream<char> olostream;


/// Stream that reads and writes a large object
/** Use this class exactly as you would a std::iostream to read data from, or
 * write data to a large object.  All formatting and streaming operations of 
 * std::iostream are supported.  What you'll typically want to use, however, is
 * the lostream typedef defined below.  This is similar to e.g. std::fstream.
 *
 * Currently only works for <char, std::char_traits<char> >.
 */
template<typename CHAR=char, typename TRAITS=PGSTD::char_traits<CHAR> > 
class basic_lostream : public PGSTD::basic_iostream<CHAR, TRAITS>
{
public:
  typedef CHAR char_type;
  typedef TRAITS traits_type;
  typedef typename traits_type::int_type int_type;
  typedef typename traits_type::pos_type pos_type;
  typedef typename traits_type::off_type off_type;

  /// Create a basic_lostream
  /** @param T transaction in which this stream is to exist
   * @param O a large object to access
   * @param BufSize size of buffer to use internally (optional)
   */
  basic_lostream(TransactionItf &T, 
      		 LargeObject O,
		 LargeObject::size_type BufSize=512) :			//[]
    PGSTD::basic_iostream<CHAR,TRAITS>(&m_Buf),
    m_Buf(T, O, in | out, BufSize) 
  { 
  }

  /// Create a basic_lostream
  /** @param T transaction in which this stream is to exist
   * @param O a large object to access
   * @param BufSize size of buffer to use internally (optional)
   */
  basic_lostream(TransactionItf &T, 
      		 Oid O,
		 LargeObject::size_type BufSize=512) :			//[]
    PGSTD::basic_iostream<CHAR,TRAITS>(&m_Buf),
    m_Buf(T, O, in | out, BufSize) 
  { 
  }

  ~basic_lostream() { m_Buf.pubsync(); m_Buf.pubsync(); }

private:
  largeobject_streambuf<CHAR,TRAITS> m_Buf;
};

typedef basic_lostream<char> lostream;

}

#endif

