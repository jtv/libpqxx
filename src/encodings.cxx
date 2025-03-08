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


namespace pqxx::internal
{
/// Convert libpq encoding name to its libpqxx encoding group.
encoding_group enc_group(std::string_view encoding_name)
{
  struct mapping
  {
  private:
    std::string_view m_name;
    encoding_group m_group;

  public:
    constexpr mapping(std::string_view n, encoding_group g) :
            m_name{n}, m_group{g}
    {}
    constexpr bool operator<(mapping const &rhs) const
    {
      return m_name < rhs.m_name;
    }
    [[nodiscard]] std::string_view get_name() const { return m_name; }
    [[nodiscard]] encoding_group get_group() const { return m_group; }
  };

  // C++20: Once compilers are ready, go full constexpr, leave to the compiler.
  auto const sz{std::size(encoding_name)};
  if (sz > 0u)
    switch (encoding_name[0])
    {
    case 'B':
      if (encoding_name == "BIG5"sv)
        return encoding_group::BIG5;
      [[unlikely]] break;
    case 'E':
      if (encoding_name.starts_with("EUC_"sv))
      {
        auto const subtype{encoding_name.substr(4)};
        static constexpr std::array<mapping, 5> subtypes{
          mapping{"CN"sv, encoding_group::EUC_CN},
          // We support EUC_JIS_2004 and EUC_JP as identical encodings.
          mapping{"JIS_2004"sv, encoding_group::EUC_JP},
          mapping{"JP"sv, encoding_group::EUC_JP},
          mapping{"KR"sv, encoding_group::EUC_KR},
          mapping{"TW"sv, encoding_group::EUC_TW},
        };
        for (auto const &m : subtypes)
          if (m.get_name() == subtype)
            return m.get_group();
      }
      [[unlikely]] break;
    case 'G':
      if (encoding_name == "GB18030"sv)
        return encoding_group::GB18030;
      else if (encoding_name == "GBK"sv)
        return encoding_group::GBK;
      [[unlikely]] break;
    case 'I':
      // We know iso-8859-X, where 5 <= X < 9.  They're all monobyte encodings.
      if (encoding_name.starts_with("ISO_8859_"sv))
      {
        char const subtype{encoding_name[9]};
        if (('5' <= subtype) and (subtype < '9'))
          return encoding_group::MONOBYTE;
      }
      [[unlikely]] break;
    case 'J':
      if (encoding_name == "JOHAB"sv)
        return encoding_group::JOHAB;
      [[unlikely]] break;
    case 'K':
      if ((encoding_name == "KOI8R"sv) or (encoding_name == "KOI8U"sv))
        return encoding_group::MONOBYTE;
      [[unlikely]] break;
    case 'L':
      // We know LATIN1 through LATIN10.
      if (encoding_name.starts_with("LATIN"sv))
      {
        auto const subtype{encoding_name.substr(5)};
        if (subtype.size() == 1)
        {
          char const n{subtype[0]};
          if (('1' <= n) and (n <= '9'))
            return encoding_group::MONOBYTE;
        }
        else if (subtype == "10"sv)
        {
          return encoding_group::MONOBYTE;
        }
      }
      [[unlikely]] break;
    case 'M':
      if (encoding_name == "MULE_INTERNAL"sv)
        return encoding_group::MULE_INTERNAL;
      [[unlikely]] break;
    case 'S':
      if (encoding_name == "SHIFT_JIS_2004"sv)
        return encoding_group::SJIS;
      else if (encoding_name == "SJIS"sv)
        return encoding_group::SJIS;
      else if (encoding_name == "SQL_ASCII"sv)
        return encoding_group::MONOBYTE;
      [[unlikely]] break;
    case 'U':
      if (encoding_name == "UHC"sv)
        return encoding_group::UHC;
      else if (encoding_name == "UTF8"sv) [[likely]]
        return encoding_group::UTF8;
      [[unlikely]] break;
    case 'W':
      if (encoding_name.substr(0, 3) == "WIN"sv)
      {
        auto const subtype{encoding_name.substr(3)};
        static constexpr std::array<std::string_view, 11u> subtypes{
          "866"sv,  "874"sv,  "1250"sv, "1251"sv, "1252"sv, "1253"sv,
          "1254"sv, "1255"sv, "1256"sv, "1257"sv, "1258"sv,
        };
        for (auto const n : subtypes)
          if (n == subtype)
            return encoding_group::MONOBYTE;
      }
      [[unlikely]] break;
    default: [[unlikely]] break;
    }
  [[unlikely]] throw std::invalid_argument{
    std::format("Unrecognized encoding: '{}'.", to_string(encoding_name))};
}


PQXX_PURE char const *name_encoding(int encoding_id)
{
  return pg_encoding_to_char(encoding_id);
}


encoding_group enc_group(int libpq_enc_id, sl)
{
  // TODO: Can we safely do this without using string representation?
  return enc_group(name_encoding(libpq_enc_id));
}


PQXX_PURE glyph_scanner_func *get_glyph_scanner(encoding_group enc, sl loc)
{
#define CASE_GROUP(ENC)                                                       \
  case encoding_group::ENC: return glyph_scanner<encoding_group::ENC>::call

  switch (enc)
  {
  case encoding_group::UNKNOWN:
    throw usage_error{"Trying to read text in unknown encoding.", loc};

    [[likely]] CASE_GROUP(MONOBYTE);
    CASE_GROUP(BIG5);
    CASE_GROUP(EUC_CN);
    CASE_GROUP(EUC_JP);
    CASE_GROUP(EUC_KR);
    CASE_GROUP(EUC_TW);
    CASE_GROUP(GB18030);
    CASE_GROUP(GBK);
    CASE_GROUP(JOHAB);
    CASE_GROUP(MULE_INTERNAL);
    CASE_GROUP(SJIS);
    CASE_GROUP(UHC);
    [[likely]] CASE_GROUP(UTF8);
  }
  throw usage_error{
    std::format("Unsupported encoding group code {}.", to_string(enc)), loc};

#undef CASE_GROUP
}
} // namespace pqxx::internal
