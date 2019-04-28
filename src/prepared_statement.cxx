/** Helper classes for defining and executing prepared statements>
 *
 * See the connection_base hierarchy for more about prepared statements.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/connection_base"
#include "pqxx/prepared_statement"
#include "pqxx/result"
#include "pqxx/transaction_base"


pqxx::prepare::internal::prepared_def::prepared_def(const std::string &def) :
  definition{def}
{
}
