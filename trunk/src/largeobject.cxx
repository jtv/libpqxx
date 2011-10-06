/*-------------------------------------------------------------------------
 *
 *   FILE
 *	largeobject.cxx
 *
 *   DESCRIPTION
 *      Implementation of the Large Objects interface
 *   Allows access to large objects directly, or though I/O streams
 *
 * Copyright (c) 2003-2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include <cerrno>
#include <stdexcept>

#include "libpq-fe.h"

//#include "libpq/libpq-fs.h"
/// Copied from libpq/libpq-fs.h so we don't need that header anymore
#define INV_WRITE		0x00020000
/// Copied from libpq/libpq-fs.h so we don't need that header anymore
#define INV_READ		0x00040000

#include "pqxx/largeobject"

#include "pqxx/internal/gates/connection-largeobject.hxx"

using namespace PGSTD;
using namespace pqxx::internal;

namespace
{

inline int StdModeToPQMode(ios::openmode mode)
{
  return ((mode & ios::in)  ? INV_READ  : 0) |
         ((mode & ios::out) ? INV_WRITE : 0);
}


inline int StdDirToPQDir(ios::seekdir dir) throw ()
{
  // TODO: Figure out whether seekdir values match C counterparts!
#ifdef PQXX_SEEKDIRS_MATCH_C
  return dir;
#else
  int pqdir;
  switch (dir)
  {
  case ios::beg: pqdir=SEEK_SET; break;
  case ios::cur: pqdir=SEEK_CUR; break;
  case ios::end: pqdir=SEEK_END; break;

  /* Added mostly to silence compiler warning, but also to help compiler detect
   * cases where this function can be optimized away completely.  This latter
   * reason should go away as soon as PQXX_SEEKDIRS_MATCH_C works.
   */
  default: pqdir = dir; break;
  }
  return pqdir;
#endif
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
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Could not create large object: " + Reason(err));
  }
}


pqxx::largeobject::largeobject(dbtransaction &T, const PGSTD::string &File) :
  m_ID()
{
  m_ID = lo_import(RawConnection(T), File.c_str());
  if (m_ID == oid_none)
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Could not import file '" + File + "' to large object: " +
	Reason(err));
  }
}


pqxx::largeobject::largeobject(const largeobjectaccess &O) throw () :
  m_ID(O.id())
{
}


void pqxx::largeobject::to_file(dbtransaction &T,
	const PGSTD::string &File) const
{
  if (lo_export(RawConnection(T), id(), File.c_str()) == -1)
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Could not export large object " + to_string(m_ID) + " "
	                "to file '" + File + "': " +
			Reason(err));
  }
}


void pqxx::largeobject::remove(dbtransaction &T) const
{
  if (lo_unlink(RawConnection(T), id()) == -1)
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Could not delete large object " + to_string(m_ID) + ": " +
	Reason(err));
  }
}


pqxx::internal::pq::PGconn *pqxx::largeobject::RawConnection(
	const dbtransaction &T)
{
  return gate::connection_largeobject(T.conn()).RawConnection();
}


string pqxx::largeobject::Reason(int err) const
{
  if (err == ENOMEM) return "Out of memory";
  if (id() == oid_none) return "No object selected";

  char buf[500];
  return string(strerror_wrapper(err, buf, sizeof(buf)));
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
					   const PGSTD::string &File,
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
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Error seeking in large object: " + Reason(err));
  }

  return Result;
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cseek(off_type dest, seekdir dir) throw ()
{
  return lo_lseek(RawConnection(), m_fd, int(dest), StdDirToPQDir(dir));
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cwrite(const char Buf[], size_type Len) throw ()
{
  return
    PGSTD::max(
	lo_write(RawConnection(), m_fd,const_cast<char *>(Buf), size_t(Len)),
        -1);
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::cread(char Buf[], size_type Bytes) throw ()
{
  return PGSTD::max(lo_read(RawConnection(), m_fd, Buf, size_t(Bytes)), -1);
}


pqxx::largeobjectaccess::pos_type
pqxx::largeobjectaccess::ctell() const throw ()
{
  return
#if defined(PQXX_HAVE_LO_TELL)
        lo_tell(RawConnection(), m_fd);
#else
        lo_lseek(RawConnection(), m_fd, 0, SEEK_CUR);
#endif
}


void pqxx::largeobjectaccess::write(const char Buf[], size_type Len)
{
  const long Bytes = cwrite(Buf, Len);
  if (Bytes < Len)
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    if (Bytes < 0)
      throw failure("Error writing to large object #" + to_string(id()) + ": " +
	Reason(err));
    if (Bytes == 0)
      throw failure("Could not write to large object #" + to_string(id()) +
	": " + Reason(err));

    throw failure("Wanted to write " + to_string(Len) + " bytes "
	"to large object #" + to_string(id()) + "; "
	"could only write " + to_string(Bytes));
  }
}


pqxx::largeobjectaccess::size_type
pqxx::largeobjectaccess::read(char Buf[], size_type Len)
{
  const long Bytes = cread(Buf, Len);
  if (Bytes < 0)
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Error reading from large object #" + to_string(id()) +
	": " + Reason(err));
  }
  return Bytes;
}


void pqxx::largeobjectaccess::open(openmode mode)
{
  m_fd = lo_open(RawConnection(), id(), StdModeToPQMode(mode));
  if (m_fd < 0)
  {
    const int err = errno;
    if (err == ENOMEM) throw bad_alloc();
    throw failure("Could not open large object " + to_string(id()) + ": " +
	Reason(err));
  }
}


void pqxx::largeobjectaccess::close() throw ()
{
#ifdef PQXX_QUIET_DESTRUCTORS
  quiet_errorhandler quiet(m_Trans.conn());
#endif
  if (m_fd >= 0) lo_close(RawConnection(), m_fd);
}


pqxx::largeobjectaccess::size_type pqxx::largeobjectaccess::tell() const
{
  const size_type res = ctell();
  if (res == -1) throw failure(Reason(errno));
  return res;
}


string pqxx::largeobjectaccess::Reason(int err) const
{
  return (m_fd == -1) ? "No object opened" : largeobject::Reason(err);
}


void pqxx::largeobjectaccess::process_notice(const PGSTD::string &s) throw ()
{
  m_Trans.process_notice(s);
}
