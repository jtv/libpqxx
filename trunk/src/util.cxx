/*-------------------------------------------------------------------------
 *
 *   FILE
 *	util.cxx
 *
 *   DESCRIPTION
 *      Various utility functions for libpqxx
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
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#ifdef PQXX_HAVE_SYS_SELECT_H
#include <sys/select.h>
#else
#include <sys/types.h>
#if defined(_WIN32)
#include <winsock2.h>
#endif
#endif // PQXX_HAVE_SYS_SELECT_H

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

#include "libpq-fe.h"

#include "pqxx/except"
#include "pqxx/util"


using namespace PGSTD;
using namespace pqxx::internal;


const char pqxx::internal::sql_begin_work[] = "BEGIN",
      pqxx::internal::sql_commit_work[] = "COMMIT",
      pqxx::internal::sql_rollback_work[] = "ROLLBACK";


pqxx::thread_safety_model pqxx::describe_thread_safety() throw ()
{
  thread_safety_model model;

#if defined(PQXX_HAVE_STRERROR_R) || defined(PQXX_HAVE_STRERROR_S)
  model.have_safe_strerror = true;
#else
  model.have_safe_strerror = false;
  model.description += "The available strerror() may not be thread-safe.\n";
#endif

#ifdef PQXX_HAVE_PQISTHREADSAFE
  if (PQisthreadsafe())
  {
    model.safe_libpq = true;
  }
  else
  {
    model.safe_libpq = false;
    model.description += "Using a libpq build that is not thread-safe.\n";
  }
#else
  model.safe_libpq = false;
  model.description +=
	"Built with old libpq version; can't tell whether it is thread-safe.\n";
#endif

#ifdef PQXX_HAVE_PQCANCEL
  model.safe_query_cancel = true;
#else
  model.safe_query_cancel = false;
  model.description +=
	"Built with old libpq version; canceling queries is unsafe..\n";
#endif

#ifdef PQXX_HAVE_SHARED_PTR
  model.safe_result_copy = true;
#else
  model.safe_result_copy = false;
  model.description +=
	"Built without shared_ptr.  Copying & destroying results is unsafe.\n";
#endif

  // Sadly I'm not aware of any way to avoid this just yet.
  model.safe_kerberos = false;
  model.description +=
	"Kerberos is not thread-safe.  If your application uses Kerberos, "
	"protect all calls to Kerberos or libpqxx using a global lock.\n";

  return model;
}


#ifndef PQXX_HAVE_SHARED_PTR
pqxx::internal::refcount::refcount() :
  m_l(this),
  m_r(this)
{
}


pqxx::internal::refcount::~refcount()
{
  loseref();
}


void pqxx::internal::refcount::makeref(refcount &rhs) throw ()
{
  // TODO: Make threadsafe
  m_l = &rhs;
  m_r = rhs.m_r;
  m_l->m_r = m_r->m_l = this;
}


bool pqxx::internal::refcount::loseref() throw ()
{
  // TODO: Make threadsafe
  const bool result = (m_l == this);
  m_r->m_l = m_l;
  m_l->m_r = m_r;
  m_l = m_r = this;
  return result;
}
#endif // PQXX_HAVE_SHARED_PTR


string pqxx::internal::namedclass::description() const
{
  try
  {
    string desc = classname();
    if (!name().empty()) desc += " '" + name() + "'";
    return desc;
  }
  catch (const exception &)
  {
    // Oops, string composition failed!  Probably out of memory.
    // Let's try something easier.
  }
  return name().empty() ? classname() : name();
}


void pqxx::internal::CheckUniqueRegistration(const namedclass *New,
    const namedclass *Old)
{
  if (!New)
    throw internal_error("NULL pointer registered");
  if (Old)
  {
    if (Old == New)
      throw usage_error("Started twice: " + New->description());
    throw usage_error("Started " + New->description() + " "
		      "while " + Old->description() + " still active");
  }
}


void pqxx::internal::CheckUniqueUnregistration(const namedclass *New,
    const namedclass *Old)
{
  if (New != Old)
  {
    if (!New)
      throw usage_error("Expected to close " + Old->description() + ", "
			"but got NULL pointer instead");
    if (!Old)
      throw usage_error("Closed while not open: " + New->description());
    throw usage_error("Closed " + New->description() + "; "
		      "expected to close " + Old->description());
  }
}


void pqxx::internal::freepqmem(const void *p) throw ()
{
  PQfreemem(const_cast<void *>(p));
}


void pqxx::internal::freemallocmem(const void *p) throw ()
{
  free(const_cast<void *>(p));
}


void pqxx::internal::sleep_seconds(int s)
{
  if (s <= 0) return;

#if defined(PQXX_HAVE_SLEEP)
  // Use POSIX.1 sleep() if available
  sleep(unsigned(s));
#elif defined(_WIN32)
  // Windows has its own Sleep(), which speaks milliseconds
  Sleep(s*1000);
#else
  // If all else fails, use select() on nothing and specify a timeout
  fd_set F;
  FD_ZERO(&F);
  struct timeval timeout;
  timeout.tv_sec = s;
  timeout.tv_usec = 0;
  if (select(0, &F, &F, &F, &timeout) == -1) switch (errno)
  {
  case EINVAL:	// Invalid timeout
	throw range_error("Invalid timeout value: " + to_string(s));
  case EINTR:	// Interrupted by signal
	break;
  case ENOMEM:	// Out of memory
	throw bad_alloc();
  default:
    throw internal_error("select() failed for unknown reason");
  }
#endif
}


#if !defined(PQXX_HAVE_STRERROR_R) || !defined(PQXX_HAVE_STRERROR_R_GNU)
namespace
{
void cpymsg(char buf[], const char input[], size_t buflen) throw ()
{
#if defined(PQXX_HAVE_STRLCPY)
  strlcpy(buf, input, buflen);
#else
  strncpy(buf, input, buflen);
  if (buflen) buf[buflen-1] = '\0';
#endif
}
}
#endif


cstring pqxx::internal::strerror_wrapper(int err, char buf[], PGSTD::size_t len)
	throw ()
{
  if (!buf || len <= 0) return "No buffer provided for error message!";

  const char *res = buf;

#if !defined(PQXX_HAVE_STRERROR_R) && !defined(PQXX_HAVE_STRERROR_S)
  cpymsg(buf, strerror(err), len);
#elif defined(PQXX_HAVE_STRERROR_R_GNU)
  // GNU strerror_r returns error string (which may be anywhere).
  return strerror_r(err, buf, len);
#elif defined(PQXX_HAVE_STRERROR_S)
  // Windows equivalent of strerror_r returns result code.
  if (strerror_s(buf, len, err) == 0) res = buf;
  else cpymsg(buf, "Unknown error", len);
#else
  // Single Unix Specification version of strerror_r returns result code.
  switch (strerror_r(err, buf, len))
  {
  case 0: res = buf; break;
  case -1: cpymsg(buf, "Unknown error", len); break;
  default:
    cpymsg(
	buf,
	"Unexpected result from strerror_r()!  Is it really SUS-compliant?",
	len);
    break;
  }
#endif
  return res;
}
