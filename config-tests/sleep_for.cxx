// Test for std::this_thread::sleep_for().
/* For some reason MinGW's <thread> header seems to be broken.
 *
 * But it gets worse.  It looks as if we can include <thread> without problems
 * in this configuration test.  Why does it break when MinGW users try to build
 * the library, but succeed when we try it here?
 *
 * To try and get close to the situation in the library code itself, we try
 * including some standard headers and OS headers that we don't strictly need
 * here.
 */

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#include <chrono>
#include <thread>

#if __has_include(<winsock2.h>)
#  include <winsock2.h>
#endif
#if __has_include(<ws2tcpip.h>)
#  include <ws2tcpip.h>
#endif
#if __has_include(<mstcpip.h>)
#  include <mstcpip.h>
#endif


int main()
{
  std::this_thread::sleep_for(std::chrono::microseconds{10u});
}
