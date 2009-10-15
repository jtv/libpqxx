/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/statement_parameters.hxx
 *
 *   DESCRIPTION
 *      Common implementation for statement parameter lists.
 *   These are used for both prepared statements and parameterized statements.
 *   DO NOT INCLUDE THIS FILE DIRECTLY.  Other headers include it for you.
 *
 * Copyright (c) 2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PQXX_H_STATEMENT_PARAMETER
#define PQXX_H_STATEMENT_PARAMETER

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include <string>
#include <vector>

#include "pqxx/strconv"
#include "pqxx/util"


namespace pqxx
{
namespace internal
{
class PQXX_LIBEXPORT statement_parameters
{
protected:
  statement_parameters();

  void add_param() { add_checked_param("", false); }
  template<typename T> void add_param(const T &v) { add_param(v, true); }
  template<typename T> void add_param(const T &v, bool nonnull)
  {
    nonnull = (nonnull && !pqxx::string_traits<T>::is_null(v));
    add_checked_param((nonnull ? to_string(v) : ""), nonnull);
  }

  /// Marshall parameter values into C-style arrays for passing to libpq.
  int marshall(
	scoped_array<const char *> &values,
	scoped_array<int> &lengths) const;

private:
  // Not allowed
  statement_parameters &operator=(const statement_parameters &);

  void add_checked_param(const PGSTD::string &, bool nonnull);

  PGSTD::vector<PGSTD::string> m_values;
  PGSTD::vector<bool> m_nonnull;
};
} // namespace pqxx::internal
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif
