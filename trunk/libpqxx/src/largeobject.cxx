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

inline int StdModeToPQMode(ios_base::openmode mode)
{
  return ((mode & ios_base::in)  ? INV_READ  : 0) |
         ((mode & ios_base::out) ? INV_WRITE : 0);
}


inline int StdDirToPQDir(ios_base::seekdir dir)
{
  int pqdir;
  switch (dir)
  {
  case PGSTD::ios_base::beg: pqdir=SEEK_SET; break;
  case PGSTD::ios_base::cur: pqdir=SEEK_CUR; break;
  case PGSTD::ios_base::end: pqdir=SEEK_END; break;

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


pqxx::LargeObject::LargeObject(TransactionItf &T) :
  m_ID()
{
  m_ID = lo_creat(RawConnection(T), INV_READ|INV_WRITE);
  if (m_ID == InvalidOid)
    throw runtime_error("Could not create large object: " +
	                string(strerror(errno)));
}


pqxx::LargeObject::LargeObject(TransactionItf &T, const string &File) :
  m_ID()
{
  m_ID = lo_import(RawConnection(T), File.c_str());
  if (m_ID == InvalidOid)
    throw runtime_error("Could not import file '" + File + "' "
	                "to large object: " + strerror(errno));
}


pqxx::LargeObject::LargeObject(const LargeObjectAccess &O) :
  m_ID(O.id())
{
}


void pqxx::LargeObject::to_file(TransactionItf &T, const char File[]) const
{
  if (lo_export(RawConnection(T), m_ID, File) == -1)
    throw runtime_error("Could not export large object " + ToString(m_ID) + " "
	                "to file '" + File + "': " +
			strerror(errno));
}


int pqxx::LargeObject::cunlink(TransactionItf &T) const throw ()
{
  return lo_unlink(RawConnection(T), id());
}



pqxx::LargeObjectAccess::LargeObjectAccess(TransactionItf &T,
                                           ios_base::openmode mode) :
  LargeObject(T),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::LargeObjectAccess(TransactionItf &T,
    					   Oid O,
					   ios_base::openmode mode) :
  LargeObject(O),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::LargeObjectAccess(TransactionItf &T,
    					   LargeObject O,
					   ios_base::openmode mode) :
  LargeObject(O),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


pqxx::LargeObjectAccess::LargeObjectAccess(TransactionItf &T,
					   const string &File,
					   ios_base::openmode mode) :
  LargeObject(T, File),
  m_Trans(T),
  m_fd(-1)
{
  open(mode);
}


long pqxx::LargeObjectAccess::cseek(long dest, ios_base::seekdir dir) throw ()
{
  return lo_lseek(RawConnection(), m_fd, dest, StdDirToPQDir(dir));
}


long pqxx::LargeObjectAccess::cwrite(const char Buf[], size_t Len) throw ()
{
  return lo_write(RawConnection(), m_fd, const_cast<char *>(Buf), Len);
}


long pqxx::LargeObjectAccess::cread(char Buf[], size_t Bytes) throw ()
{
  return lo_read(RawConnection(), m_fd, Buf, Bytes);
}


void pqxx::LargeObjectAccess::open(ios_base::openmode mode)
{
  m_fd = lo_open(RawConnection(), id(), StdModeToPQMode(mode));
  if (m_fd < 0)
    throw runtime_error("Could not open large object " + ToString(id()) + ": " +
	                strerror(errno));
}


void pqxx::LargeObjectAccess::close()
{
  if (m_fd >= 0) lo_close(RawConnection(), m_fd);
}

