/*-------------------------------------------------------------------------
 *
 *   FILE
 *	largeobject.cxx
 *
 *   DESCRIPTION
 *      Implementation of the Large Objects interface
 *   Allows access to large objects directly, or though I/O streams
 *
 * Copyright (c) 2003-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <cerrno>
#include <stdexcept>

#include "libpq/libpq-fs.h"

#include "pqxx/largeobject"

using namespace PGSTD;
using namespace pqxx::internal::pq;

namespace
{

inline int StdModeToPQMode(ios::openmode mode)
{
  return ((mode & ios::in)  ? INV_READ  : 0) |
         ((mode & ios::out) ? INV_WRITE : 0);
}


inline int StdDirToPQDir(ios::seekdir dir) throw ()
{
  int pqdir;
  switch (dir)
  {
  case ios::beg: pqdir=SEEK_SET; break;
  case ios::cur: pqdir=SEEK_CUR; break;
  case ios::end: pqdir=SEEK_END; break;

  /* Default clause added for two reasons: one, to silence compiler warning
   * about values not handled in switch (due to tackiness in std headers), and
   * two, to help the optimizer recognize implementations where the numerical
   * values of dir and pqdir are always equal.
   */
  default: pqdir = dir; break;
  }

  return pqdir;
}

} // namespace


pqxx::largeobject::largeobject() throw () :
  m_ID(oid_none)
{
}


pqxx::largeobject::largeobject(dbtransaction &T) :
  m_ID()
{
  m_ID = lo_creat(RawConnection(T), INV_READ|INV_WRITE);
  if (m_ID == oid_none)
    throw runtime_error("Could not create large object: " +
	                string(strerror(errno)));
}


pqxx::largeobject::largeobject(dbtransaction &T, const string &File) :
  m_ID()
{
  m_ID = lo_import(RawConnection(T), File.c_str());
  if (m_ID == oid_none)
  {
    const int err = errno;
    throw runtime_error("Could not import file '" + File + "' "
	                "to large object: " + strerror(err));
  }
}


pqxx::largeobject::largeobject(const largeobjectaccess &O) throw () :
  m_ID(O.id())
{
}


void pqxx::largeobject::to_file(dbtransaction &T, const string &File) const
{
  if (lo_export(RawConnection(T), id(), File.c_str()) == -1)
    throw runtime_error("Could not export large object " + to_string(m_ID) + " "
	                "to file '" + File + "': " +
			Reason());
}


void pqxx::largeobject::remove(dbtransaction &T) const
{
  if (lo_unlink(RawConnection(T), id()) == -1)
    throw runtime_error("Could not delete large object " + 
	                to_string(m_ID) + ": " +
			Reason());
}


string pqxx::largeobject::Reason() const
{
  return (id() == oid_none) ? "No object selected" : strerror(errno);
}


pqxx::largeobjectaccess::largeobjectaccess(dbtransaction &T, openmode mode) :
  largeobject(T),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(dbtransaction &T,
    					   oid O,
					   openmode mode) :
  largeobject(O),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(dbtransaction &T,
    					   largeobject O,
					   openmode mode) :
  largeobject(O),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::largeobjectaccess::largeobjectaccess(dbtransaction &T,
					   const string &File,
					   openmode mode) :
  largeobject(T, File),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::largeobjectaccess::size_type 
pqxx::largeobjectaccess::seek(size_type dest, seekdir dir)
{
  const size_type Result = cseek(dest, dir);
  if (Result == -1)
    throw runtime_error("Error seeking in large object: " + Reason()); 

  return Result;
}


long pqxx::largeobjectaccess::cseek(off_type dest, seekdir dir) throw ()
{
  return lo_lseek(RawConnection(), m_fd, dest, StdDirToPQDir(dir));
}


long pqxx::largeobjectaccess::cwrite(const char Buf[], size_type Len) throw ()
{
  return
    PGSTD::max(lo_write(RawConnection(),m_fd,const_cast<char *>(Buf), Len), -1);
}


long pqxx::largeobjectaccess::cread(char Buf[], size_type Bytes) throw ()
{
  return PGSTD::max(lo_read(RawConnection(), m_fd, Buf, Bytes), -1);
}


void pqxx::largeobjectaccess::write(const char Buf[], size_type Len)
{
  const long Bytes = cwrite(Buf, Len);
  if (Bytes < Len)
  {
    if (Bytes < 0)
      throw runtime_error("Error writing to large object "
                          "#" + to_string(id()) + ": " +
	                  Reason());
    if (Bytes == 0)
      throw runtime_error("Could not write to large object #" + 
	                  to_string(id()) + ": " + Reason());

    throw runtime_error("Wanted to write " + to_string(Len) + " bytes "
	                "to large object #" + to_string(id()) + "; "
			"could only write " + to_string(Bytes));
  }
}


pqxx::largeobjectaccess::size_type 
pqxx::largeobjectaccess::read(char Buf[], size_type Len)
{
  const long Bytes = cread(Buf, Len);
  if (Bytes < 0)
    throw runtime_error("Error reading from large object #" + to_string(id()) +
	                ": " + Reason());
  return Bytes;
}


void pqxx::largeobjectaccess::open(openmode mode)
{
  m_fd = lo_open(RawConnection(), id(), StdModeToPQMode(mode));
  if (m_fd < 0)
    throw runtime_error("Could not open large object " + to_string(id()) + ":"
			" " + Reason());
}


void pqxx::largeobjectaccess::close() throw ()
{
  if (m_fd >= 0) lo_close(RawConnection(), m_fd);
}


string pqxx::largeobjectaccess::Reason() const
{
  return (m_fd == -1) ? "No object opened" : largeobject::Reason();
}


void pqxx::largeobjectaccess::process_notice(const string &s) throw ()
{
  m_Trans.process_notice(s);
}

