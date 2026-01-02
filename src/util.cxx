/** Various utility functions.
 *
 * Copyright (c) 2000-2026, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <array>
#include <cassert>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <new>

#include "pqxx/internal/header-pre.hxx"

extern "C"
{
#include <libpq-fe.h>
}

#include "pqxx/except.hxx"
#include "pqxx/util.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;

PQXX_COLD pqxx::thread_safety_model pqxx::describe_thread_safety()
{
  auto const libpq_ok{PQisthreadsafe() != 0}, kerb_ok{false};

  return {
    .description = std::format(
      "{}{}",
      (libpq_ok ? "" : "Using a libpq build that is not thread-safe.\n"),
      (kerb_ok ?
         "" :
         "Kerberos is not thread-safe.  If your application uses Kerberos, "
         "protect all calls to Kerberos or libpqxx using a global lock.\n")),
    .safe_libpq = libpq_ok,
    // Sadly I'm not aware of any way to avoid this just yet.
    .safe_kerberos = kerb_ok};
}


std::string pqxx::internal::describe_object(
  std::string_view class_name, std::string_view obj_name)
{
  if (std::empty(obj_name))
    return std::string{class_name};
  else
    return std::format("{} '{}'", class_name, obj_name);
}


void pqxx::internal::check_unique_register(
  void const *old_guest, std::string_view old_class, std::string_view old_name,
  void const *new_guest, std::string_view new_class, std::string_view new_name)
{
  if (new_guest == nullptr)
    throw internal_error{"Null pointer registered."};

  if (old_guest != nullptr)
    throw usage_error{
      (old_guest == new_guest) ?
        std::format(
          "Started twice: {}.", describe_object(old_class, old_name)) :
        std::format(
          "Started new {} while {} was still active.",
          describe_object(new_class, new_name),
          describe_object(old_class, old_name))};
}


void pqxx::internal::check_unique_unregister(
  void const *old_guest, std::string_view old_class, std::string_view old_name,
  void const *new_guest, std::string_view new_class, std::string_view new_name)
{
  if (new_guest != old_guest) [[unlikely]]
  {
    if (new_guest == nullptr)
      throw usage_error{std::format(
        "Expected to close, but got null pointer instead.",
        describe_object(old_class, old_name))};
    if (old_guest == nullptr)
      throw usage_error{std::format(
        "Closed while not open: {}", describe_object(new_class, new_name))};
    else
      throw usage_error{std::format(
        "Closed {}; expected to close ", describe_object(new_class, new_name),
        describe_object(old_class, old_name))};
  }
}


namespace
{
constexpr std::array<char, 16u> hex_digits{
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};


// LCOV_EXCL_START
/// Translate a number (must be between 0 and 16 exclusive) to a hex digit.
constexpr char hex_digit(int c) noexcept
{
  PQXX_ASSUME(c >= 0);
  return hex_digits.at(static_cast<unsigned int>(c));
}


#if !defined(NDEBUG)
static_assert(hex_digit(0) == '0');
static_assert(hex_digit(9) == '9');
static_assert(hex_digit(10) == 'a');
static_assert(hex_digit(15) == 'f');
#endif
// LCOV_EXCL_STOP


constexpr int ten{10};


// LCOV_EXCL_START
/// Translate a hex digit to a nibble.  Return -1 if it's not a valid digit.
constexpr int nibble(int c) noexcept
{
  if (c >= '0' and c <= '9') [[likely]]
    return c - '0';
  else if (c >= 'a' and c <= 'f')
    return ten + (c - 'a');
  else [[unlikely]] if (c >= 'A' and c <= 'F')
    return ten + (c - 'A');
  else [[unlikely]]
    return -1;
}


#if !defined(NDEBUG)
static_assert(nibble('0') == 0);
static_assert(nibble('9') == 9);
static_assert(nibble('a') == 10);
static_assert(nibble('f') == 15);
#endif
// LCOV_EXCL_STOP
} // namespace


void pqxx::internal::esc_bin(
  bytes_view binary_data, std::span<char> buffer) noexcept
{
  assert(std::size(buffer) >= size_esc_bin(std::size(binary_data)));
  std::size_t here{0u};
  // C++26: Use at().
  buffer[here++] = '\\';
  buffer[here++] = 'x';

  constexpr int nibble_bits{4};
  constexpr int nibble_mask{0x0f};
  for (auto const byte : binary_data)
  {
    auto uc{static_cast<unsigned char>(byte)};

    buffer[here++] = hex_digit(uc >> nibble_bits);
    buffer[here++] = hex_digit(uc & nibble_mask);
  }

  // (No need to increment further.  Facebook's "infer" complains if we do.)
  buffer[here] = '\0';
}


std::string pqxx::internal::esc_bin(bytes_view binary_data)
{
  auto const bytes{size_esc_bin(std::size(binary_data))};
  std::string buf;
  buf.resize(bytes);
  esc_bin(binary_data, buf);
  // Strip off the trailing zero.
  buf.resize(bytes - 1);
  return buf;
}


void pqxx::internal::unesc_bin(
  std::string_view escaped_data, std::span<std::byte> buffer, sl loc)
{
  auto const in_size{std::size(escaped_data)};
  if (in_size < 2)
    throw pqxx::failure{"Binary data appears truncated.", loc};
  if ((in_size % 2) != 0)
    throw pqxx::failure{"Invalid escaped binary length.", loc};
  std::size_t in{0u};
  if (escaped_data[in++] != '\\' or escaped_data[in++] != 'x')
    throw pqxx::failure(
      "Escaped binary data did not start with '\\x'`.  Is the server or libpq "
      "too old?",
      loc);
  std::size_t out{0u};
  while (in < in_size)
  {
    int const hi{nibble(escaped_data[in++])};
    if (hi < 0)
      throw pqxx::failure{"Invalid hex-escaped data.", loc};
    int const lo{nibble(escaped_data[in++])};
    if (lo < 0)
      throw pqxx::failure{"Invalid hex-escaped data.", loc};
    buffer[out++] = static_cast<std::byte>((hi << 4) | lo);
  }
  assert(out <= std::size(buffer));
}


pqxx::bytes pqxx::internal::unesc_bin(std::string_view escaped_data, sl loc)
{
  auto const bytes{size_unesc_bin(std::size(escaped_data))};
  pqxx::bytes buf;
  buf.resize(bytes);
  unesc_bin(escaped_data, buf, loc);
  return buf;
}


namespace pqxx::internal::pq
{
void pqfreemem(void const *ptr) noexcept
{
  // Why is it OK to const_cast here?  Because this is the C equivalent to a
  // destructor.  Those apply to const objects as well as non-const ones.
  // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
  PQfreemem(const_cast<void *>(ptr));
}
} // namespace pqxx::internal::pq
