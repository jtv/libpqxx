/* Definition of the pqxx::errorhandler class.
 *
 * pqxx::errorhandler handlers errors and warnings in a database session.
 *
 * DO NOT INCLUDE THIS FILE DIRECTLY; include pqxx/errorhandler instead.
 *
 * Copyright (c) 2000-2024, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_H_ERRORHANDLER
#define PQXX_H_ERRORHANDLER

#if !defined(PQXX_HEADER_PRE)
#  error "Include libpqxx headers as <pqxx/header>, not <pqxx/header.hxx>."
#endif

#include "pqxx/types.hxx"


namespace pqxx::internal::gate
{
class errorhandler_connection;
}


namespace pqxx
{
/**
 * @addtogroup errorhandler
 */
//@{

/// @deprecated Base class for obsolete error-handler callbacks.
/** This method of handling errors is obsolete.  Use a "notice handler"
 * instead.
 *
 * @warning Strange things happen when a result object outlives its parent
 * connection.  If you register an error handler on a connection, then you must
 * not access the result after destroying the connection.  This applies even if
 * you destroy the error handler first!
 */
class PQXX_LIBEXPORT errorhandler
{
public:
  [[deprecated("Use a notice handler instead.")]]
  explicit errorhandler(connection &);
  virtual ~errorhandler();

  /// Define in subclass: receive an error or warning message from the
  /// database.
  /**
   * @return Whether the same error message should also be passed to the
   * remaining, older errorhandlers.
   */
  virtual bool operator()(char const msg[]) noexcept = 0;

  errorhandler() = delete;
  errorhandler(errorhandler const &) = delete;
  errorhandler &operator=(errorhandler const &) = delete;

private:
  connection *m_home;

  friend class internal::gate::errorhandler_connection;
  void unregister() noexcept;
};


/// @deprecated Use a notice handler instead.
class quiet_errorhandler : public errorhandler
{
public:
#include "pqxx/internal/ignore-deprecated-pre.hxx"
  /// Suppress error notices.
  [[deprecated("Use notice handlers instead.")]]
  quiet_errorhandler(connection &cx) :
          errorhandler{cx}
  {}
#include "pqxx/internal/ignore-deprecated-post.hxx"

  /// Revert to previous handling of error notices.
  virtual bool operator()(char const[]) noexcept override { return false; }
};

//@}
} // namespace pqxx
#endif
