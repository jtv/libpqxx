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
/// Translate a number (must be between 0 and 16 exclusive) to a hex digit.
constexpr char hex_digit(int c) noexcept
{
  constexpr char hex[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                          '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'};
  return hex[c];
}


/// Translate a hex digit to a nibble.  Return -1 if it's not a valid digit.
constexpr int nibble(int c) noexcept
{
  if (c >= '0' and c <= '9')
    return c - '0';
  else if (c >= 'a' and c <= 'f')
    return 10 + (c - 'a');
  else if (c >= 'A' and c <= 'F')
    return 10 + (c - 'A');
  else
    return -1;
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


std::string pqxx::internal::esc_bin(std::string_view binary_data)
{
  auto const bytes{size_esc_bin(binary_data.size())};
  std::string buf;
  buf.resize(bytes);
  esc_bin(binary_data, buf.data());
  // Strip off the trailing zero.
  buf.resize(bytes - 1);
  return buf;
}


void pqxx::internal::unesc_bin(
  std::string_view escaped_data, unsigned char buffer[])
{
  auto const in_size{escaped_data.size()};
  if (in_size < 2)
    throw pqxx::failure{"Binary data appears truncated."};
  if ((in_size % 2) != 0)
    throw pqxx::failure{"Invalid escaped binary length."};
  char const *in{escaped_data.data()};
  char const *const end{in + in_size};
  if (*in++ != '\\' or *in++ != 'x')
    throw pqxx::failure(
      "Escaped binary data did not start with '\\x'`.  Is the server or libpq "
      "too old?");
  auto out{buffer};
  while (in != end)
  {
    int hi{nibble(*in++)};
    if (hi < 0)
      throw pqxx::failure{"Invalid hex-escaped data."};
    int lo{nibble(*in++)};
    if (lo < 0)
      throw pqxx::failure{"Invalid hex-escaped data."};
    *out++ = static_cast<unsigned char>((hi << 4) | lo);
  }
}


std::string pqxx::internal::unesc_bin(std::string_view escaped_data)
{
  auto const bytes{size_unesc_bin(escaped_data.size())};
  std::string buf;
  buf.resize(bytes);
  unesc_bin(escaped_data, reinterpret_cast<unsigned char *>(buf.data()));
  return buf;
}
