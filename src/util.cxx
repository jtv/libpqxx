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
#include "pqxx/version.hxx"

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
} // namespace


namespace pqxx::internal
{
void esc_bin(bytes_view binary_data, std::span<char> buffer) noexcept
{
  // clang-tidy rule bug:
  // NOLINTNEXTLINE(cert-dcl03-c)
  assert(std::size(buffer) >= size_esc_bin(std::size(binary_data)));
  std::size_t here{0u};
  // C++26: Use at().
  buffer[here++] = '\\';
  buffer[here++] = 'x';

  constexpr int nibble_bits{4};
  constexpr int nibble_mask{0x0f};
  for (auto const byte : binary_data)
  {
    auto const uc{static_cast<unsigned char>(byte)};

    buffer[here++] = hex_digit(uc >> nibble_bits);
    buffer[here++] = hex_digit(uc & nibble_mask);
  }

  // (No need to increment further.  Facebook's "infer" complains if we do.)
  buffer[here] = '\0';
}


std::string esc_bin(bytes_view binary_data)
{
  auto const bytes{size_esc_bin(std::size(binary_data))};
  std::string buf;
  buf.resize(bytes);
  esc_bin(binary_data, buf);
  // Strip off the trailing zero.
  buf.resize(bytes - 1);
  return buf;
}


void unesc_bin(
  std::string_view escaped_data, std::span<std::byte> buffer, sl loc)
{
  // Size of the header: "\x".
  constexpr std::size_t header{2u};
  // Number of hex digits in a byte.
  constexpr std::size_t digits_per_byte{2u};

  auto const in_size{std::size(escaped_data)};
  if (in_size < header)
    throw pqxx::failure{"Binary data appears truncated.", loc};
  if ((in_size % digits_per_byte) != 0)
    throw pqxx::failure{"Invalid escaped binary length.", loc};
  // NOLINTNEXTLINE(modernize-raw-string-literal)
  if (not escaped_data.starts_with("\\x"sv))
    throw pqxx::failure("Escaped binary data did not start with '\\x'`.", loc);

  // Read in chunks that can fit in this type.
  using word_t = unsigned long long;

  // Number of bytes in word_t.
  constexpr std::size_t word_bytes{sizeof(word_t)};

  // Number of hex digits that can fit into that word.
  constexpr std::size_t word_digits{sizeof(word_t) * digits_per_byte};

  // The actual hex digits portion of the input.
  std::string_view const hex_data{escaped_data.substr(header)};

  // Pointer to the hex data.
  auto const hex_ptr{std::data(hex_data)};

  // Number of words we can process in chunks.  We do the remainder separately.
  std::size_t const bulk_limit{
    (std::size(hex_data) / word_digits) * word_digits};

  static_assert(
    (std::endian::native == std::endian::big) or
    (std::endian::native == std::endian::little));

  // Our current byte offests in hex_data and buffer, respectively.
  std::size_t read{0u}, write{0u};

  // This loop does some repeated calculations and such, but compilers are
  // good at optimising those away.
  while (read < bulk_limit)
  {
    // The chunk of binary data, as a single numeric data.
    word_t chunk{};

    // Extent of this chunk in hex_data.
    auto const chunk_start{read}, chunk_end{read + word_digits};

    // Read a whole chunk of data, abusing the standard library for fast
    // parsing of hex digits.
    auto const conv_res{
      std::from_chars(hex_ptr + chunk_start, hex_ptr + chunk_end, chunk, 16)};
    if (
      (conv_res.ptr != (hex_ptr + chunk_end)) or (conv_res.ec != std::errc{}))
      throw conversion_error{"Invalid hex-escaped binary data.", loc};

    // Write binary chunk to buffer.  It would be tempting to extract the
    // most-significant byte, write it, then shift left.  But it's much
    // cheaper to extract the least-significant byte, so we simply write
    // backwards.
    //
    // I hope the compiler will make something good of this.
    for (std::size_t i{word_bytes}; i > 0; --i)
    {
      buffer[write + i - 1] = static_cast<std::byte>(chunk & 0xff);
      chunk >>= 8;
    }

    read += word_digits;
    write += word_bytes;
  }

  // Convert those few (if any) remaining bytes, one byte (2 digits) at a time.
  while (read < std::size(hex_data))
  {
    unsigned int chunk{};
    auto const chunk_start{read}, chunk_end{read + digits_per_byte};
    auto const conv_res{
      std::from_chars(hex_ptr + chunk_start, hex_ptr + chunk_end, chunk, 16)};
    if ((conv_res.ptr != hex_ptr + chunk_end) or (conv_res.ec != std::errc{}))
      throw conversion_error{"Invalid hex-escaped binary data.", loc};

    read += digits_per_byte;
    buffer[write++] = static_cast<std::byte>(chunk & 0xff);
  }

  assert(write == std::size(buffer));
  assert(read == std::size(hex_data));
}


pqxx::bytes unesc_bin(std::string_view escaped_data, sl loc)
{
  auto const bytes{size_unesc_bin(std::size(escaped_data))};
  pqxx::bytes buf;
  buf.resize(bytes);
  unesc_bin(escaped_data, buf, loc);
  return buf;
}


/// Persistent copies of libpqxx version information.
/** These are not efficient.  They are stored here for one reason only: to
 * ensure that the libpqxx library binary contains its own version information,
 * even if the application code that calls libpqxx is compiled against a
 * different version.  (We allow some slight differences.)  That way we can
 * compare the binary version against the version that was in the headers.
 *
 * The `volatile` is probably overkill.  I just want this to have linkage as
 * far as compilation is concerned.
 */
volatile int const binary_major{version_major}, binary_minor{version_minor},
  binary_patch{version_patch};


/// Persistent copy of libpqxx version string.
/** I'd like to make this `volatile`, but there are just too many complications
 * along the way.  Which is probably for the best.
 *
 * Let's just hope that this declaration is enough to prevent the compiler from
 * optimising this away completely.  (I wouldn't particularly mind if the
 * linker did so though.)
 */
std::string_view const binary_version{version};


int check_libpqxx_version(
  int apps_major, int apps_minor, int apps_patch,
  std::string_view apps_version)
{
  if ((apps_major == binary_major) and (apps_minor == binary_minor))
  {
    // Major and minor versions match.  Without that, the odds of even getting
    // to this point in the code are pretty slim anyway.

    if (0 <= apps_patch)
    {
      // Application was compiled against a "regular" libpqxx version.  We're
      // cool so long as the libpqxx binary is also regular, and the binary's
      // patch level is no less than the one used in the application build.
      //
      // (If 0 <= apps_patch <= version_patch, then it follows that
      // 0 <= version_patch.  No need to check that.)
      if (apps_patch <= binary_patch)
        return 1;
    }
    else
    {
      // Compiled against a "special" libpqxx version, such as a release
      // candidate.  We don't try to get clever about tolerating mismatches in
      // such cases; there's just too much that can go wrong.  Demand an exact
      // string match.
      if (apps_version == binary_version)
        return 2;
    }
  }

  throw version_mismatch{std::format(
    "Mismatch in libpqxx versions: Compiled against libpqxx {} headers, "
    "but libpqxx binary is {}.",
    apps_version, binary_version)};
}
} // namespace pqxx::internal


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
