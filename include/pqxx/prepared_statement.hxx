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
/**
 * @name Prepared statements
 *
 * These are very similar to parameterised statements.  The difference is
 * that you prepare a statement in advance, before you execute it, giving it an
 * identifying name.  You can then call it by this name, as many times as you
 * like, passing in separate sets of argument values appropriate for each call.
 *
 * You prepare a statement on the connection, using
 * @ref pqxx::connection::prepare().  But you then call the statement in a
 * transaction, by calling
 * @ref pqxx::transaction_base::exec(pqxx::prepped, pqxx::params).
 *
 * The @ref pqxx::prepped type is really just a zero-terminated string, but
 * wrapped in its own type.  This type only exists for one reason: it indicates
 * that the string is not an SQL statement itself, but the _name_ of a prepared
 * statement.
 *
 * See \ref prepared for a full discussion.
 *
 * @warning Beware of "nul" bytes.  Any string you pass as a parameter will
 * end at the first char with value zero.  If you pass a string that contains
 * a zero byte, the last byte in the value will be the one just before the
 * zero.  If you need a zero byte, you're dealing with binary strings, not
 * regular strings.  Represent binary strings on the SQL side as `BYTEA`
 * (or as large objects).  On the C++ side, use types like `pqxx::bytes` or
 * `pqxx::bytes_view` or (in C++20) `std::vector<std::byte>`.  Also, consider
 * large objects on the SQL side and @ref blob on the C++ side.
 *
 * @warning Passing the wrong number of parameters to a prepared or
 * parameterised statement will _break the connection._  The usual exception
 * that occurs in this situation is @ref pqxx::protocol_violation.  It's a
 * subclass of @ref pqxx::broken_connection, but where `broken_connection`
 * usually indicates a networking problem, `protocol_violation` indicates
 * that the communication with the server has deviated from protocol.  Once
 * something like that happens, communication is broken and there is nothing
 * for it but to discard the connection.  A networking problem is usually
 * worth retrying, but a protocol violation is not.  Once the two ends of the
 * connection disagree about where they are in their conversation, they can't
 * get back on track.  This is beyond libpqxx's control.
 */
//@{

/// A string that is the name of a prepared statement.
/**
 * When calling on libpqxx to execute a prepared statement, wrap its name in
 * a `prepped` object to indicate to libpqxx that it is a statement name, not
 * SQL.
 *
 * The string must be like a C-style string: it should contain no bytes with
 * value zero, but it must have a single byte with value zero directly behind
 * it in memory.
 */
class PQXX_LIBEXPORT prepped : public zview
{
public:
  // TODO: May not have to be a zview!  Because exec() draws a copy anyway.
  prepped(zview name) : zview{name} {}
};
} // namespace pqxx

#endif
