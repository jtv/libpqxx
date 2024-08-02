/* Definition of the pqxx::prepped.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/prepared_statement instead.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_PREPARED_STATEMENT
#define PQXX_H_PREPARED_STATEMENT

namespace pqxx
{
/// A string that is the name of a prepared statement.
/** When calling on libpqxx to execute a prepared statement, wrap its name in
 * a `prepped` object to indicate to libpqxx that it is a statement name, not
 * SQL.
 */
class PQXX_LIBEXPORT prepped : public zview
{
public:
  prepped(zview name) : zview{name} {}
};
} // namespace pqxx

#endif
