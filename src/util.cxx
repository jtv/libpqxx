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


/// The standard header for PostgreSQL's hex-escaped binary data.
constexpr std::string_view header{"\\x"sv};

/// Number of hex digits in a byte.
constexpr std::size_t digits_per_byte{2u};


/// Sanity-check hex input, return "payload" portion (the hex digits part).
inline std::string_view parse_bin_meta(
  std::string_view escaped_data, std::span<std::byte> buffer, sl loc)
{
  // NOLINTNEXTLINE(modernize-raw-string-literal)
  if (not escaped_data.starts_with(header))
    throw pqxx::conversion_error("Encoded binary data is malformed.", loc);

  std::string_view const payload{escaped_data.substr(std::size(header))};

  if ((std::size(payload) % digits_per_byte) != 0)
    throw pqxx::conversion_error{"Invalid encoded binary length.", loc};

  auto const need_bytes{std::size(payload) / digits_per_byte},
    have_bytes{std::size(buffer)};

  if (need_bytes > have_bytes)
    throw pqxx::conversion_overrun{std::format(
      "Need buffer of {} byte(s) to decode binary data, got {}.", need_bytes,
      have_bytes)};

  return payload;
}


/// Marker value for invalid hex digit.
/** A hex digit value really only needs one nibble (4 bits).  But we "poison"
 * this value by setting extra bits.  This lets us skip a lot of per-digit
 * validity checking and just check at the end whether our digits contain
 * bits of poison.
 */
constexpr uint16_t bad_hex{0xffff};

// X/// Does the given value contain `bad_hex` poison?
[[nodiscard]] inline constexpr bool has_poison(unsigned value) noexcept
{
  // Are any of the bits outside the single-byte range set?
  return (value >> 8) != 0;
}


static_assert(has_poison(bad_hex));
static_assert(not has_poison(0));
static_assert(not has_poison(255));


// XXX: Also check that streaming uses fast from_string(outvar) path!
/// Hex digit lookup table.
class hex_table final
{
public:
  constexpr hex_table() noexcept
  {
    for (unsigned i{0}; i < std::size(m_digits); ++i) m_digits[i] = bad_hex;
    for (unsigned i{0}; i < 10u; ++i)
      m_digits['0' + i] = static_cast<uint16_t>(i);
    for (unsigned i{0}; i < 6u; ++i)
    {
      std::uint16_t const value{static_cast<std::uint16_t>(10u + i)};
      m_digits['A' + i] = value;
      m_digits['a' + i] = value;
    }
  }

  [[nodiscard]] constexpr unsigned operator[](char c) const noexcept
  {
    return m_digits[static_cast<unsigned char>(c)];
  }

private:
  std::array<uint16_t, 256> m_digits;
};


constexpr hex_table hex_digits;

static_assert(hex_digits['`'] == bad_hex);
static_assert(hex_digits['0'] == 0);
static_assert(hex_digits['9'] == 9);
static_assert(hex_digits[':'] == bad_hex);
static_assert(hex_digits['@'] == bad_hex);
static_assert(hex_digits['A'] == 10);
static_assert(hex_digits['a'] == 10);
static_assert(hex_digits['F'] == 15);
static_assert(hex_digits['f'] == 15);
static_assert(hex_digits['G'] == bad_hex);
static_assert(hex_digits['g'] == bad_hex);


void unesc_bin(
  std::string_view escaped_data, std::span<std::byte> buffer, sl loc)
{
  std::string_view const hex_data{parse_bin_meta(escaped_data, buffer, loc)};
  auto const in_size{std::size(hex_data)};

  // Collect all values in here, so we can later check for poison in one go.
  unsigned chalice{0};

  std::size_t write{0};

  // This is the central loop.  It matters a lot for performance, so avoid any
  // conditionals etc.
  for (std::size_t read{0}; read < in_size; read += digits_per_byte)
  {
    // Can we use std pairwise iteration, maybe get more vectorisation?
    unsigned byteval =
      ((hex_digits[hex_data[read]] << 4) | hex_digits[hex_data[read + 1]]);
    chalice |= byteval;
    buffer[write++] = static_cast<std::byte>(byteval);
  }

  if (has_poison(chalice))
    throw conversion_error{
      "Hex-encoded binary data contains invalid digits.", loc};
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
