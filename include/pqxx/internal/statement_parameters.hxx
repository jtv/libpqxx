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
 * Copyright (c) 2009-2016, Jeroen T. Vermeulen <jtv@xs4all.nl>
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

#include "pqxx/binarystring"
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

  void add_param() { this->add_checked_param("", false, false); }
  template<typename T> void add_param(const T &v, bool nonnull)
  {
    nonnull = (nonnull && !pqxx::string_traits<T>::is_null(v));
    this->add_checked_param(
	(nonnull ? pqxx::to_string(v) : ""),
	nonnull,
	false);
  }
  void add_binary_param(const binarystring &b, bool nonnull)
	{ this->add_checked_param(b.str(), nonnull, true); }

  /// Marshall parameter values into C-compatible arrays for passing to libpq.
  int marshall(
	std::vector<const char *> &values,
	std::vector<int> &lengths,
	std::vector<int> &binaries) const;

private:
  // Not allowed
  statement_parameters &operator=(const statement_parameters &);

  void add_checked_param(const std::string &, bool nonnull, bool binary);

  std::vector<std::string> m_values;
  std::vector<bool> m_nonnull;
  std::vector<bool> m_binary;
};
} // namespace pqxx::internal
} // namespace pqxx

#include "pqxx/compiler-internal-post.hxx"

#endif
