/*-------------------------------------------------------------------------
 *
 *   FILE
 *	largeobject.cxx
 *
 *   DESCRIPTION
 *      Implementation of the Large Objects interface
 *   Allows access to large objects directly, or though I/O streams
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <cerrno>
#include <stdexcept>

#include "libpq/libpq-fs.h"

#include "pqxx/largeobject.h"

using namespace PGSTD;

namespace
{

inline int StdModeToPQMode(ios::openmode mode)
{
  return ((mode & ios::in)  ? INV_READ  : 0) |
         ((mode & ios::out) ? INV_WRITE : 0);
}


inline int StdDirToPQDir(ios::seekdir dir)
{
  int pqdir;
  switch (dir)
  {
  case PGSTD::ios::beg: pqdir=SEEK_SET; break;
  case PGSTD::ios::cur: pqdir=SEEK_CUR; break;
  case PGSTD::ios::end: pqdir=SEEK_END; break;

  /* Default clause added for two reasons: one, to silence compiler warning
   * about values not handled in switch (due to tackiness in std headers), and
   * two, to help the optimizer recognize implementations where the numerical
   * values of dir and pqdir are always equal.
   */
  default: pqdir = dir; break;
  }

  return pqdir;
}

const Oid pqxxInvalidOid(InvalidOid);

} // namespace


pqxx::LargeObject::LargeObject() :
  m_ID(pqxxInvalidOid)
{
}


pqxx::LargeObject::LargeObject(Transaction_base &T) :
  m_ID()
{
  m_ID = lo_creat(RawConnection(T), INV_READ|INV_WRITE);
  if (m_ID == pqxxInvalidOid)
    throw runtime_error("Could not create large object: " +
	                string(strerror(errno)));
}


pqxx::LargeObject::LargeObject(Transaction_base &T, const string &File) :
  m_ID()
{
  m_ID = lo_import(RawConnection(T), File.c_str());
  if (m_ID == pqxxInvalidOid)
    throw runtime_error("Could not import file '" + File + "' "
	                "to large object: " + strerror(errno));
}


pqxx::LargeObject::LargeObject(const LargeObjectAccess &O) :
  m_ID(O.id())
{
}


void pqxx::LargeObject::to_file(Transaction_base &T, const string &File) const
{
  if (lo_export(RawConnection(T), id(), File.c_str()) == -1)
    throw runtime_error("Could not export large object " + ToString(m_ID) + " "
	                "to file '" + File + "': " +
			Reason());
}


void pqxx::LargeObject::remove(Transaction_base &T) const
{
  if (lo_unlink(RawConnection(T), id()) == -1)
    throw runtime_error("Could not delete large object " + 
	                ToString(m_ID) + ": " +
			Reason());
}


string pqxx::LargeObject::Reason() const
{
  return (id() == pqxxInvalidOid) ? "No object selected" : strerror(errno);
}


pqxx::LargeObjectAccess::LargeObjectAccess(Transaction_base &T,
                                           openmode mode) :
  LargeObject(T),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::LargeObjectAccess(Transaction_base &T,
    					   Oid O,
					   openmode mode) :
  LargeObject(O),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::LargeObjectAccess(Transaction_base &T,
    					   LargeObject O,
					   openmode mode) :
  LargeObject(O),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::LargeObjectAccess(Transaction_base &T,
					   const string &File,
					   openmode mode) :
  LargeObject(T, File),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::size_type 
pqxx::LargeObjectAccess::seek(size_type dest, seekdir dir)
{
  const size_type Result = cseek(dest, dir);
  if (Result == -1)
    throw runtime_error("Error seeking in large object: " + Reason()); 

  return Result;
}


long pqxx::LargeObjectAccess::cseek(long dest, seekdir dir) throw ()
{
  return lo_lseek(RawConnection(), m_fd, dest, StdDirToPQDir(dir));
}


long pqxx::LargeObjectAccess::cwrite(const char Buf[], size_type Len) throw ()
{
  return max(lo_write(RawConnection(), m_fd, const_cast<char *>(Buf), Len), -1);
}


long pqxx::LargeObjectAccess::cread(char Buf[], size_type Bytes) throw ()
{
  return max(lo_read(RawConnection(), m_fd, Buf, Bytes), -1);
}


void pqxx::LargeObjectAccess::write(const char Buf[], size_type Len)
{
  const long Bytes = cwrite(Buf, Len);
  if (Bytes < Len)
  {
    if (Bytes < 0)
      throw runtime_error("Error writing to large object "
                          "#" + ToString(id()) + ": " +
	                  Reason());
    if (Bytes == 0)
      throw runtime_error("Could not write to large object #" + 
	                  ToString(id()) + ": " + Reason());

    throw runtime_error("Wanted to write " + ToString(Len) + " bytes "
	                "to large object #" + ToString(id()) + "; "
			"could only write " + ToString(Bytes));
  }
}


pqxx::LargeObjectAccess::size_type 
pqxx::LargeObjectAccess::read(char Buf[], size_type Len)
{
  const long Bytes = cread(Buf, Len);
  if (Bytes < 0)
    throw runtime_error("Error reading from large object #" + ToString(id()) +
	                ": " + Reason());
  return Bytes;
}


void pqxx::LargeObjectAccess::open(openmode mode)
{
  m_fd = lo_open(RawConnection(), id(), StdModeToPQMode(mode));
  if (m_fd < 0)
    throw runtime_error("Could not open large object " + ToString(id()) + ": " +
	                Reason());
}


void pqxx::LargeObjectAccess::close()
{
  if (m_fd >= 0) lo_close(RawConnection(), m_fd);
}


string pqxx::LargeObjectAccess::Reason() const
{
  return (m_fd == -1) ? "No object opened" : LargeObject::Reason();
}

