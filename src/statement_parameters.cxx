/*-------------------------------------------------------------------------
 *
 *   FILE
 *	statement_parameters.cxx
 *
 *   DESCRIPTION
 *      Common implementation for statement parameter lists.
 *   See the connection_base hierarchy for more about prepared statements
 *
 * Copyright (c) 2006-2016, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


pqxx::internal::statement_parameters::statement_parameters() :
  m_values(),
  m_nonnull()
{
}


void pqxx::internal::statement_parameters::add_checked_param(
	const std::string &v,
	bool nonnull,
	bool binary)
{
  m_nonnull.push_back(nonnull);
  if (nonnull) m_values.push_back(v);
  m_binary.push_back(binary);
}


int pqxx::internal::statement_parameters::marshall(
	std::vector<const char *> &values,
	std::vector<int> &lengths,
	std::vector<int> &binaries) const
{
  const size_t elements = m_nonnull.size();
  const size_t array_size = elements + 1;
  values.clear();
  values.resize(array_size, NULL);
  lengths.clear();
  lengths.resize(array_size, 0);
  // "Unpack" from m_values, which skips arguments that are null, to the
  // outputs which represent all parameters including nulls.
  size_t arg = 0;
  for (size_t param = 0; param < elements; ++param)
    if (m_nonnull[param])
    {
      values[param] = m_values[arg].c_str();
      lengths[param] = int(m_values[arg].size());
      ++arg;
    }

  // The binaries array is simpler: it maps 1-on-1.
  binaries.resize(array_size);
  for (size_t param = 0; param < elements; ++param)
    binaries[param] = int(m_binary[param]);
  binaries.back() = 0;

  return int(elements);
}
