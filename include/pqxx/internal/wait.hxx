#if !defined(PQXX_WAIT_HXX)
#  define PQXX_WAIT_HXX

#  include "pqxx/types.hxx"


namespace pqxx::internal
{
/// Wait.
/** This is normally `std::this_thread::sleep_for()`.  But MinGW's `thread`
 * header doesn't work, so we must be careful about including it.
 */
PQXX_LIBEXPORT void wait_for(unsigned int microseconds);


/// Wait for a socket to be ready for reading/writing, or timeout.
/** All waits are finite.  It is not guaranteed that this will wait for the
 * full time specified, e.g. because the process receives a signal.  The
 * calling code MUST be prepared to handle an occasional premature return.
 *
 * _Why does this not wait forever?_  Mainly so that you do not forget to
 * handle premature returns, and write subtly buggy code that appears to work
 * fine in testing!
 *
 * _Okay, but why don't you _handle_ signals and continue to wait?_  Because
 * how the application handles signals is the application's business.  It's not
 * up to libpqxx to mess with that.  Perhaps you _want_ to return early after
 * some particular signal.  Perhaps you don't for some other signal.
 *
 * _But what if I need to minimise wakeups to save power or CPU time?_  It
 * probably won't make much of a difference whether it's just libpqxx waking up
 * or your own code waking up as well.  What you can do is pass a longer wait
 * time, so it happens less often.
 *
 * Increasing wait times can help, but there are diminishing returns.  It will
 * reduce this particular overhead in your application, until some other source
 * of overhead becomes more significant.  At that point... stop increasing the
 * wait time and go fix that other thing!
 */
PQXX_LIBEXPORT void wait_fd(
  int fd, bool for_read, bool for_write, unsigned seconds = 1,
  unsigned microseconds = 0, sl = sl::current());
} // namespace pqxx::internal
#endif
