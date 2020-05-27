/** Implementation of bytea (binary string) conversions.
 *
 * Copyright (c) 2000-2020, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <new>
#include <stdexcept>

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/binarystring"
#include "pqxx/field"


namespace
{
/// Copy data to a heap-allocated buffer.
std::shared_ptr<unsigned char>
copy_to_buffer(void const *data, std::size_t len)
{
  void *const output{malloc(len + 1)};
  if (output == nullptr)
    throw std::bad_alloc{};
  static_cast<char *>(output)[len] = '\0';
  memcpy(static_cast<char *>(output), data, len);
  return std::shared_ptr<unsigned char>{
    static_cast<unsigned char *>(output), std::free};
}
} // namespace


pqxx::binarystring::binarystring(field const &F)
{
  unsigned char const *data{
    reinterpret_cast<unsigned char const *>(F.c_str())};
  m_buf =
    std::shared_ptr<unsigned char>{PQunescapeBytea(data, &m_size), PQfreemem};
  if (m_buf == nullptr)
    throw std::bad_alloc{};
}


pqxx::binarystring::binarystring(std::string_view s) :
        m_buf{copy_to_buffer(s.data(), s.size())}, m_size{s.size()}
{}


pqxx::binarystring::binarystring(void const *binary_data, std::size_t len) :
        m_buf{copy_to_buffer(binary_data, len)}, m_size{len}
{}


bool pqxx::binarystring::operator==(binarystring const &rhs) const noexcept
{
  return (rhs.size() == size()) and
         (std::memcmp(data(), rhs.data(), size()) == 0);
}


pqxx::binarystring &
pqxx::binarystring::operator=(binarystring const &rhs) = default;

pqxx::binarystring::const_reference pqxx::binarystring::at(size_type n) const
{
  if (n >= m_size)
  {
    if (m_size == 0)
      throw std::out_of_range{"Accessing empty binarystring"};
    throw std::out_of_range{
      "binarystring index out of range: " + to_string(n) +
      " (should be below " + to_string(m_size) + ")"};
  }
  return data()[n];
}


void pqxx::binarystring::swap(binarystring &rhs)
{
  m_buf.swap(rhs.m_buf);

  // This part very obviously can't go wrong, so do it last
  auto const s{m_size};
  m_size = rhs.m_size;
  rhs.m_size = s;
}


std::string pqxx::binarystring::str() const
{
  return std::string{get(), m_size};
}
