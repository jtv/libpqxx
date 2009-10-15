/*-------------------------------------------------------------------------
 *
 *   FILE
 *	statement_parameters.cxx
 *
 *   DESCRIPTION
 *      Common implementation for statement parameter lists.
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2009, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/util"

#include "pqxx/internal/statement_parameters.hxx"

using namespace PGSTD;


pqxx::internal::statement_parameters::statement_parameters() :
  m_values(),
  m_nonnull()
{
}


void pqxx::internal::statement_parameters::add_checked_param(
	const PGSTD::string &v,
	bool nonnull)
{
  m_nonnull.push_back(nonnull);
  if (nonnull) try
  {
    m_values.push_back(v);
  }
  catch (const exception &)
  {
    // Might as well be exception-safe. 
    m_nonnull.resize(m_nonnull.size() - 1);
    throw;
  }
}


int pqxx::internal::statement_parameters::marshall(
	scoped_array<const char *> &values,
	scoped_array<int> &lengths) const
{
  const size_t elements = m_nonnull.size();
  values = new const char *[elements+1];
  lengths = new int[2*(elements+1)];
  int v = 0;
  for (size_t i = 0; i < elements; ++i)
  {
    if (m_nonnull[i])
    {
      values[i] = m_values[v].c_str();
      lengths[i] = int(m_values[v].size());
      ++v;
    }
    else
    {
      values[i] = 0;
      lengths[i] = 0;
    }
  }

  values[elements] = 0;
  lengths[elements] = 0;

  return int(elements);
}
