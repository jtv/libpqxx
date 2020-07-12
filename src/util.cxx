/** Various utility functions.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/except"
#include "pqxx/util"


pqxx::thread_safety_model pqxx::describe_thread_safety()
{
  thread_safety_model model;

  if (PQisthreadsafe() == 0)
  {
    model.safe_libpq = false;
    model.description += "Using a libpq build that is not thread-safe.\n";
  }
  else
  {
    model.safe_libpq = true;
  }

  // Sadly I'm not aware of any way to avoid this just yet.
  model.safe_kerberos = false;
  model.description +=
    "Kerberos is not thread-safe.  If your application uses Kerberos, "
    "protect all calls to Kerberos or libpqxx using a global lock.\n";

  return model;
}


std::string pqxx::internal::namedclass::description() const
{
  std::string const cname{classname()};
  if (name().empty())
    return cname;
  else
    return cname + " '" + name() + "'";
}


void pqxx::internal::check_unique_registration(
  namedclass const *new_ptr, namedclass const *old_ptr)
{
  if (new_ptr == nullptr)
    throw internal_error{"null pointer registered."};
  if (old_ptr != nullptr)
  {
    if (old_ptr == new_ptr)
      throw usage_error{"Started twice: " + new_ptr->description()};
    throw usage_error{
      "Started " + new_ptr->description() + " while " +
      old_ptr->description() + " still active."};
  }
}


void pqxx::internal::check_unique_unregistration(
  namedclass const *new_ptr, namedclass const *old_ptr)
{
  if (new_ptr != old_ptr)
  {
    if (new_ptr == nullptr)
      throw usage_error{
        "Expected to close " + old_ptr->description() +
        ", "
        "but got null pointer instead."};
    if (old_ptr == nullptr)
      throw usage_error{"Closed while not open: " + new_ptr->description()};
    throw usage_error{
      "Closed " + new_ptr->description() +
      "; "
      "expected to close " +
      old_ptr->description()};
  }
}


namespace
{
constexpr char hex_digit(int c) noexcept
{
  constexpr char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  return hex[c];
}
} // namespace


void pqxx::internal::esc_bin(
  std::string_view binary_data, char buffer[]) noexcept
{
  auto here{buffer};
  *here++ = '\\';
  *here++ = 'x';

  for (auto const byte : binary_data)
  {
    auto uc{static_cast<unsigned char>(byte)};
    *here++ = hex_digit(uc >> 4);
    *here++ = hex_digit(uc & 0x0f);
  }

  *here++ = '\0';
}
