/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/connectionpolicy.hxx
 *
 *   DESCRIPTION
 *      definition of the connection policy classes
 *   Interface for defining connection policies
 *   DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection instead.
 *
 * Copyright (c) 2005-2012, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_CONNECTIONPOLICY
#define PQXX_H_CONNECTIONPOLICY

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <string>

#include "pqxx/internal/libpq-forward.hxx"


namespace pqxx
{


/**
 * @addtogroup connection Connection classes
 */
//@{

class PQXX_LIBEXPORT connectionpolicy
{
public:
  typedef internal::pq::PGconn *handle;

  explicit connectionpolicy(const PGSTD::string &opts);
  virtual ~connectionpolicy() PQXX_NOEXCEPT;

  const PGSTD::string &options() const PQXX_NOEXCEPT { return m_options; }

  virtual handle do_startconnect(handle orig);
  virtual handle do_completeconnect(handle orig);
  virtual handle do_dropconnect(handle orig) PQXX_NOEXCEPT;
  virtual handle do_disconnect(handle orig) PQXX_NOEXCEPT;
  virtual bool is_ready(handle) const PQXX_NOEXCEPT;

protected:
  handle normalconnect(handle);

private:
  PGSTD::string m_options;
};

//@}
} // namespace

#include "pqxx/compiler-internal-post.hxx"

#endif

