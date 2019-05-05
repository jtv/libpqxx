/** Definition of the connection classes.
 *
 * Different ways of setting up a backend connection.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/connection instead.
 *
 * Copyright (c) 2000-2019, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#ifndef PQXX_H_CONNECTION
#define PQXX_H_CONNECTION

#include "pqxx/compiler-public.hxx"
#include "pqxx/compiler-internal-pre.hxx"

#include "pqxx/connectionpolicy.hxx"
#include "pqxx/basic_connection.hxx"

namespace pqxx
{

/**
 * @addtogroup connection Connection classes
 *
 * The connection classes are where the use of a database begins.  You must
 * connect to a database in order to access it.  Your connection represents a
 * session with the database.  In the context of that connection you can create
 * transactions, which in turn you can use to execute SQL.  A connection can
 * have only one regular transaction open at a time, but you can break your work
 * down into any number of consecutive transactions and there is also some
 * support for transaction nesting (using the subtransaction class).
 *
 * Many things come together in the connection classes.  Handling of error and
 * warning messages, for example, is defined by @e errorhandlers in the context
 * of a connection.  Prepared statements are also defined here.
 *
 * @warning In libpqxx 7, all built-in connection types will be implemented
 * as a single class.  You'll specify the connection policy as an optional
 * constructor argument.
 *
 * When you create a connection, you pass details such as the database you wish
 * to connect to, username and password, and so on.  These all go into a
 * PostgreSQL "connection string".  Alternatively, there are certain environment
 * variables that you can learn more about from the core postgres documentation.
 *
 * Once you have your connection, you can also set certain session variables
 * defined by the backend to influence its behaviour for the duration of your
 * session, such as the applicable text encoding.
 *
 * @{
 */

/// Connection policy; creates an immediate connection to a database.
/** This is the policy you typically need when you work with a database through
 * libpqxx.  It connects to the database immediately.
 */
class PQXX_LIBEXPORT connect_direct : public connectionpolicy
{
public:
  /// The parsing of options is the same as in libpq's PQconnect.
  /// See: https://www.postgresql.org/docs/10/static/libpq-connect.html
  explicit connect_direct(const std::string &opts) : connectionpolicy{opts} {}
  virtual handle do_startconnect(handle) override;
};

/// The "standard" connection type: connect to database right now
using connection = basic_connection_base<connect_direct>;

/**
 * @}
 */

}

#include "pqxx/compiler-internal-post.hxx"
#endif
