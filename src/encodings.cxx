/** Implementation of string encodings support
 *
 * Copyright (c) 2000-2025, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#include "pqxx-source.hxx"

#include <cstring>
#include <map>
#include <sstream>

extern "C"
{
#include <libpq-fe.h>
}


#include "pqxx/internal/header-pre.hxx"

#include "pqxx/internal/encodings.hxx"
#include "pqxx/strconv.hxx"

#include "pqxx/internal/header-post.hxx"


using namespace std::literals;


namespace
{
/// Do these two string_views contain the same text?
constexpr bool same(std::string_view a, std::string_view b) noexcept
{
  // Clang-tidy check "readability-string-compare" prefers '==' over compare().
  return a == b;
}


/// Subtypes of Windows encoding.
const std::array<std::string_view, 11u> windows_subtypes{
  "866"sv,  "874"sv,  "1250"sv, "1251"sv, "1252"sv, "1253"sv,
  "1254"sv, "1255"sv, "1256"sv, "1257"sv, "1258"sv,
};
} // namespace


namespace pqxx::internal
{
// C++23: Reduce to const std::flat_map on top of constexpr array?
/// Look up encoding group for an encoding by name.
/** @throw argument_error if the encoding name is not an accepted one.
 */
constexpr encoding_group enc_group(std::string_view encoding_name, sl loc)
{
  auto const sz{std::size(encoding_name)};
  if (sz > 0u)
    switch (encoding_name[0])
    {
    case 'B':
      if (same(encoding_name, "BIG5"sv))
        return encoding_group::big5;
      break;
    case 'E':
      // All the EUC encodings are ASCII-safe.
      if (encoding_name.starts_with("EUC_"sv))
        return encoding_group::ascii_safe;
      break;
    case 'G':
      if (same(encoding_name, "GB18030"sv))
        return encoding_group::gb18030;
      else if (same(encoding_name, "GBK"sv))
        return encoding_group::gbk;
      break;
    case 'I':
      // We know iso-8859-X, where 5 <= X < 9.  They're all single-byte
      // encodings, and therefore ASCII-safe.
      if (
        (std::size(encoding_name) == 10) and
        encoding_name.starts_with("ISO_8859_"sv))
      {
        char const subtype{encoding_name[9]};
        if (('5' <= subtype) and (subtype < '9'))
          return encoding_group::ascii_safe;
      }
      break;
    case 'J':
      if (same(encoding_name, "JOHAB"sv))
        return encoding_group::johab;
      break;
    case 'K':
      if (same(encoding_name, "KOI8R"sv) or same(encoding_name, "KOI8U"sv))
        return encoding_group::ascii_safe;
      break;
    case 'L':
      // We know LATIN1 through LATIN10.
      if (encoding_name.starts_with("LATIN"sv))
      {
        auto const subtype{encoding_name.substr(5)};
        if (subtype.size() == 1)
        {
          char const n{subtype[0]};
          if (('1' <= n) and (n <= '9'))
            return encoding_group::ascii_safe;
        }
        else if (same(subtype, "10"sv))
        {
          return encoding_group::ascii_safe;
        }
      }
      break;
    case 'M':
      if (same(encoding_name, "MULE_INTERNAL"sv))
        return encoding_group::ascii_safe;
      break;
    case 'S':
      if (same(encoding_name, "SHIFT_JIS_2004"sv))
        return encoding_group::sjis;
      else if (same(encoding_name, "SJIS"sv))
        return encoding_group::sjis;
      else if (same(encoding_name, "SQL_ASCII"sv))
        return encoding_group::ascii_safe;
      break;
    case 'U':
      [[likely]] if (same(encoding_name, "UHC"sv))
        return encoding_group::uhc;
      else if (same(encoding_name, "UTF8"sv)) [[likely]]
        return encoding_group::ascii_safe;
      break;
    case 'W':
      if (same(encoding_name.substr(0, 3), "WIN"sv))
      {
        auto const subtype{encoding_name.substr(3)};
        for (auto const n : windows_subtypes)
          if (same(n, subtype))
            return encoding_group::ascii_safe;
      }
      break;
    default: break;
    }
  throw argument_error{
    std::format("Unrecognized encoding: '{}'.", encoding_name), loc};
}


// Compile-time tests.  (Conditional so as not to slow down production builds.)
#if defined(DEBUG)
static_assert(enc_group("BIG5", sl::current()) == encoding_group::big5);
static_assert(
  enc_group("EUC_CN", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("EUC_JIS_2004", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("EUC_JP", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("EUC_KR", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("EUC_TW", sl::current()) == encoding_group::ascii_safe);
static_assert(enc_group("GB18030", sl::current()) == encoding_group::gb18030);
static_assert(enc_group("GBK", sl::current()) == encoding_group::gbk);
static_assert(
  enc_group("ISO_8859_1", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("ISO_8859_2", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("ISO_8859_3", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("ISO_8859_4", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("ISO_8859_5", sl::current()) == encoding_group::ascii_safe);
static_assert(enc_group("JOHAB", sl::current()) == encoding_group::johab);
static_assert(enc_group("KOI8R", sl::current()) == encoding_group::ascii_safe);
static_assert(enc_group("KOI8U", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN1", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN2", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN3", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN4", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN5", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN6", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN7", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN8", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN9", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("LATIN10", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("MULE_INTERNAL", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("SHIFT_JIS_2004", sl::current()) == encoding_group::sjis);
static_assert(enc_group("SJIS", sl::current()) == encoding_group::sjis);
static_assert(
  enc_group("SQL_ASCII", sl::current()) == encoding_group::ascii_safe);
static_assert(enc_group("UHC", sl::current()) == encoding_group::uhc);
static_assert(enc_group("UTF8", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN866", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN874", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1250", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1251", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1252", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1253", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1254", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1255", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1256", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1257", sl::current()) == encoding_group::ascii_safe);
static_assert(
  enc_group("WIN1258", sl::current()) == encoding_group::ascii_safe);
#endif // DEBUG


PQXX_PURE char const *name_encoding(int encoding_id) noexcept
{
  return pg_encoding_to_char(encoding_id);
}


PQXX_PURE encoding_group enc_group(int libpq_enc_id, sl loc)
{
  // TODO: Is there a safe, faster way without the string representation?
  return enc_group(name_encoding(libpq_enc_id), loc);
}
} // namespace pqxx::internal
