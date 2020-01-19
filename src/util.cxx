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
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>
#include <thread>

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
  const std::string cname{classname()};
  if (name().empty())
    return cname;
  else
    return cname + " '" + name() + "'";
}


void pqxx::internal::check_unique_registration(
  const namedclass *new_ptr, const namedclass *old_ptr)
{
  if (new_ptr == nullptr)
    throw internal_error{"null pointer registered."};
  if (old_ptr != nullptr)
  {
    if (old_ptr == new_ptr)
      throw usage_error{"Started twice: " + new_ptr->description()};
    throw usage_error{"Started " + new_ptr->description() + " while " +
                      new_ptr->description() + " still active."};
  }
}


void pqxx::internal::check_unique_unregistration(
  const namedclass *new_ptr, const namedclass *old_ptr)
{
  if (new_ptr != old_ptr)
  {
    if (new_ptr == nullptr)
      throw usage_error{"Expected to close " + old_ptr->description() +
                        ", "
                        "but got null pointer instead."};
    if (old_ptr == nullptr)
      throw usage_error{"Closed while not open: " + new_ptr->description()};
    throw usage_error{"Closed " + new_ptr->description() +
                      "; "
                      "expected to close " +
                      old_ptr->description()};
  }
}


void pqxx::internal::sleep_seconds(int s)
{
  std::this_thread::sleep_for(std::chrono::seconds(s));
}
