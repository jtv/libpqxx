/** Implementation of string encodings support
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/except.hxx"
#include "pqxx/internal/encodings.hxx"

#include <iomanip>
#include <map>
#include <sstream>

using namespace pqxx::internal;

extern "C"
{
#include "libpq-fe.h"
// These headers would be needed (in this order) to use the libpq encodings enum
// directly, which the pg_wchar.h header explicitly warns against doing:
// #include "internal/c.h"
// #include "server/mb/pg_wchar.h"
}


// Internal helper functions
namespace
{

[[noreturn]] void throw_for_encoding_error(
  const char* encoding_name,
  const char* buffer,
  std::string::size_type start,
  std::string::size_type count
)
{
  std::stringstream s;
  s
    << "Invalid byte sequence for encoding "
    << encoding_name
    << " at byte "
    << start
    << ": "
    << std::hex
    << std::setw(2)
    << std::setfill('0')
  ;
  for (std::string::size_type i{0}; i < count; ++i)
  {
    s << "0x" << static_cast<unsigned int>(
      static_cast<unsigned char>(buffer[start + i])
    );
    if (i + 1 < count) s << " ";
  }
  throw pqxx::argument_error{s.str()};
}

/*
EUC-JP and EUC-JIS-2004 represent slightly different code points but iterate
the same
https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-JP
http://x0213.org/codetable/index.en.html
*/
seq_position next_seq_for_euc_jplike(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start,
  const char* encoding_name
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) == 0x8E
      && start + 2 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      )
        return {start, start + 2};
      else
        throw_for_encoding_error(encoding_name, buffer, start, 2);
    }
    else if (
         static_cast<unsigned char>(buffer[start]) >= 0xA1
      && static_cast<unsigned char>(buffer[start]) <= 0xFE
      && start + 2 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      )
        return {start, start + 2};
      else
        throw_for_encoding_error(encoding_name, buffer, start, 2);
    }
    else if (
         static_cast<unsigned char>(buffer[start]) == 0x8F
      && start + 3 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
        && static_cast<unsigned char>(buffer[start + 2]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 2]) <= 0xFE
      )
        return {start, start + 3};
      else
        throw_for_encoding_error(encoding_name, buffer, start, 3);
    }
    else
      throw_for_encoding_error(encoding_name, buffer, start, 1);
  }
}

/*
As far as I can tell, for the purposes of iterating the only difference between
SJIS and SJIS-2004 is increased range in the first byte of two-byte sequences
(0xEF increased to 0xFC).  Officially, that is; apparently the version of SJIS
used by Postgres has the same range as SJIS-2004.  They both have increased
range over the documented versions, not having the even/odd restriction for the
first byte in 2-byte sequences.
*/
// https://en.wikipedia.org/wiki/Shift_JIS#Shift_JIS_byte_map
// http://x0213.org/codetable/index.en.html
seq_position next_seq_for_sjislike(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start,
  const char* encoding_name
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (
    static_cast<unsigned char>(buffer[start]) < 0x80
    || (
         static_cast<unsigned char>(buffer[start]) >= 0xA1
      && static_cast<unsigned char>(buffer[start]) <= 0xDF
    )
  )
    return {start, start + 1};
  else
  {
    if ((
         static_cast<unsigned char>(buffer[start]) >= 0x81
      && static_cast<unsigned char>(buffer[start]) <= 0x9F
    ) || (
         static_cast<unsigned char>(buffer[start]) >= 0xE0
      // && static_cast<unsigned char>(buffer[start]) <= 0xEF
      && static_cast<unsigned char>(buffer[start]) <= 0xFC
    ))
    {
      if (start + 2 <= buffer_len)
      {
        if ((
             /*static_cast<unsigned char>(buffer[start    ]) % 2 == 1
          && */static_cast<unsigned char>(buffer[start + 1]) >= 0x40
          && static_cast<unsigned char>(buffer[start + 1]) <= 0x9E
          && static_cast<unsigned char>(buffer[start + 1]) != 0x7F
        ) || (
             /*static_cast<unsigned char>(buffer[start    ]) % 2 == 0
          && */static_cast<unsigned char>(buffer[start + 1]) >= 0x9F
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xFC
        ))
          return {start, start + 2};
        else
          throw_for_encoding_error(encoding_name, buffer, start, 2);
      }
      else
        throw_for_encoding_error(
          encoding_name,
          buffer,
          start,
          buffer_len - start
        );
    }
    else
      throw_for_encoding_error(encoding_name, buffer, start, 1);
  }
}

} // namespace


// Implement template specializations first
namespace pqxx
{
namespace internal
{

template<> seq_position next_seq<encoding_group::MONOBYTE>(
  const char* /*buffer*/,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else
    return {start, start + 1};
}

// https://en.wikipedia.org/wiki/Big5#Organization
template<> seq_position next_seq<encoding_group::BIG5>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) >= 0x81
      && static_cast<unsigned char>(buffer[start]) <= 0xFE
      && start + 2 <= buffer_len
    )
    {
      if ((
           static_cast<unsigned char>(buffer[start + 1]) >= 0x40
        && static_cast<unsigned char>(buffer[start + 1]) <= 0x7E
      ) || (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      ))
        return {start, start + 2};
      else
        throw_for_encoding_error("BIG5", buffer, start, 2);
    }
    else
      throw_for_encoding_error("BIG5", buffer, start, 1);
  }
}

/*
The PostgreSQL documentation claims that the EUC_* encodings are 1-3 bytes each,
but other documents explain that the EUC sets can contain 1-(2,3,4) bytes
depending on the specific extension:
    EUC_CN      : 1-2
    EUC_JP      : 1-3
    EUC_JIS_2004: 1-2
    EUC_KR      : 1-2
    EUC_TW      : 1-4
*/

// https://en.wikipedia.org/wiki/GB_2312#EUC-CN
template<> seq_position next_seq<encoding_group::EUC_CN>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) >= 0xA1
      && static_cast<unsigned char>(buffer[start]) <= 0xF7
      && start + 2 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      )
        return {start, start + 2};
      else
        throw_for_encoding_error("EUC_CN", buffer, start, 2);
    }
    else
      throw_for_encoding_error("EUC_CN", buffer, start, 1);
  }
}

template<> seq_position next_seq<encoding_group::EUC_JP>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  return next_seq_for_euc_jplike(buffer, buffer_len, start, "EUC_JP");
}

template<> seq_position next_seq<encoding_group::EUC_JIS_2004>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  return next_seq_for_euc_jplike(buffer, buffer_len, start, "EUC_JIS_2004");
}

// https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-KR
template<> seq_position next_seq<encoding_group::EUC_KR>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) >= 0xA1
      && static_cast<unsigned char>(buffer[start]) <= 0xFE
      && start + 2 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      )
        return {start, start + 2};
      else
        throw_for_encoding_error("EUC_KR", buffer, start, 1);
    }
    else
      throw_for_encoding_error("EUC_KR", buffer, start, 1);
  }
}

// https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-TW
template<> seq_position next_seq<encoding_group::EUC_TW>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) >= 0xA1
      && static_cast<unsigned char>(buffer[start]) <= 0xFE
      && start + 2 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      )
        return {start, start + 2};
      else
        throw_for_encoding_error("EUC_KR", buffer, start, 2);
    }
    else if (
         static_cast<unsigned char>(buffer[start]) == 0x8E
      && start + 4 <= buffer_len
    )
    {
      if (
           static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xB0
        && static_cast<unsigned char>(buffer[start + 2]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 2]) <= 0xFE
        && static_cast<unsigned char>(buffer[start + 3]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 3]) <= 0xFE
      )
        return {start, start + 4};
      else
        throw_for_encoding_error("EUC_KR", buffer, start, 4);
    }
    else
      throw_for_encoding_error("EUC_KR", buffer, start, 1);
  }
}

// https://en.wikipedia.org/wiki/GB_18030#Mapping
template<> seq_position next_seq<encoding_group::GB18030>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (
       static_cast<unsigned char>(buffer[start]) <= 0x80
    || static_cast<unsigned char>(buffer[start]) == 0xFF
  )
    return {start, start + 1};
  else
  {
    if (
         start + 2 <= buffer_len
      && static_cast<unsigned char>(buffer[start + 1]) >= 0x40
      && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
    )
    {
      if (static_cast<unsigned char>(buffer[start + 1]) != 0x7F)
        return {start, start + 2};
      else
        throw_for_encoding_error("GB18030", buffer, start, 2);
    }
    else
    {
      if (start + 4 <= buffer_len)
      {
        if (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x30
          && static_cast<unsigned char>(buffer[start + 1]) <= 0x39
          && static_cast<unsigned char>(buffer[start + 2]) >= 0x81
          && static_cast<unsigned char>(buffer[start + 2]) <= 0xFE
          && static_cast<unsigned char>(buffer[start + 3]) >= 0x30
          && static_cast<unsigned char>(buffer[start + 3]) <= 0x39
        )
          return {start, start + 4};
        else
          throw_for_encoding_error("GB18030", buffer, start, 4);
      }
      else
        throw_for_encoding_error(
          "GB18030",
          buffer,
          start,
          buffer_len - start
        );
    }
  }
}

// https://en.wikipedia.org/wiki/GBK_(character_encoding)#Encoding
template<> seq_position next_seq<encoding_group::GBK>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (start + 2 <= buffer_len)
    {
      if ((
           static_cast<unsigned char>(buffer[start    ]) >= 0xA1
        && static_cast<unsigned char>(buffer[start    ]) <= 0xA9
        && static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0xB0
        && static_cast<unsigned char>(buffer[start    ]) <= 0xF7
        && static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0x81
        && static_cast<unsigned char>(buffer[start    ]) <= 0xA0
        && static_cast<unsigned char>(buffer[start + 1]) >= 0x40
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
        && static_cast<unsigned char>(buffer[start + 1]) != 0x7F
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0xAA
        && static_cast<unsigned char>(buffer[start    ]) <= 0xFE
        && static_cast<unsigned char>(buffer[start + 1]) >= 0x40
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xA0
        && static_cast<unsigned char>(buffer[start + 1]) != 0x7F
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0xA8
        && static_cast<unsigned char>(buffer[start    ]) <= 0xA9
        && static_cast<unsigned char>(buffer[start + 1]) >= 0x40
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xA0
        && static_cast<unsigned char>(buffer[start + 1]) != 0x7F
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0xAA
        && static_cast<unsigned char>(buffer[start    ]) <= 0xAF
        && static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0xF8
        && static_cast<unsigned char>(buffer[start    ]) <= 0xFE
        && static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
      ) || (
           static_cast<unsigned char>(buffer[start    ]) >= 0xA1
        && static_cast<unsigned char>(buffer[start    ]) <= 0xA7
        && static_cast<unsigned char>(buffer[start + 1]) >= 0x40
        && static_cast<unsigned char>(buffer[start + 1]) <= 0xA0
        && static_cast<unsigned char>(buffer[start + 1]) != 0x7F
      ))
        return {start, start + 2};
      else
        throw_for_encoding_error("GBK", buffer, start, 2);
    }
    else
      throw_for_encoding_error("GBK", buffer, start, 1);
  }
}

/*
The PostgreSQL documentation claims that the JOHAB encoding is 1-3 bytes, but
"CJKV Information Processing" describes it (actually just the Hangul portion) as
"three five-bit segments" that reside inside 16 bits (2 bytes).
*/
// CJKV Information Processing by Ken Lunde, pg. 269
// https://books.google.com/books?id=SA92uQqTB-AC&pg=PA269&lpg=PA269&dq=JOHAB+encoding&source=bl&ots=GMvxWWl8Gx&sig=qLFQNkR4d7Rd-iqQy1lNh3oEdOE&hl=en&sa=X&ved=0ahUKEwizyoTDxePbAhWjpFkKHU65DSwQ6AEIajAH#v=onepage&q=JOHAB%20encoding&f=false
template<> seq_position next_seq<encoding_group::JOHAB>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (start + 2 <= buffer_len)
    {
      if ((
           static_cast<unsigned char>(buffer[start]) >= 0x84
        && static_cast<unsigned char>(buffer[start]) <= 0xD3
        && ((
             static_cast<unsigned char>(buffer[start + 1]) >= 0x41
          && static_cast<unsigned char>(buffer[start + 1]) <= 0x7E
        ) || (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x81
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
        ))
      ) || (
        ((
             static_cast<unsigned char>(buffer[start]) >= 0xD8
          && static_cast<unsigned char>(buffer[start]) <= 0xDE
        ) || (
             static_cast<unsigned char>(buffer[start]) >= 0xE0
          && static_cast<unsigned char>(buffer[start]) <= 0xF9
        ))
        && ((
             static_cast<unsigned char>(buffer[start + 1]) >= 0x31
          && static_cast<unsigned char>(buffer[start + 1]) <= 0x7E
        ) || (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x91
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
        ))
      ))
        return {start, start + 2};
      else
        throw_for_encoding_error("JOHAB", buffer, start, 2);
    }
    else
      throw_for_encoding_error("JOHAB", buffer, start, 1);
  }
}

/*
PostgreSQL's MULE_INTERNAL is the emacs rather than Xemacs implementation;
see the server/mb/pg_wchar.h PostgreSQL header file.
This is implemented according to the description in said header file, but I was
unable to get it to successfully iterate a MULE-encoded test CSV generated using
PostgreSQL 9.2.23.  Use this at your own risk.
*/
template<> seq_position next_seq<encoding_group::MULE_INTERNAL>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (start + 2 <= buffer_len)
    {
      if (
           static_cast<unsigned char>(buffer[start    ]) >= 0x81
        && static_cast<unsigned char>(buffer[start    ]) <= 0x8D
        && static_cast<unsigned char>(buffer[start + 1]) >= 0xA0
        // && static_cast<unsigned char>(buffer[start + 1]) <= 0xFF
      )
        return {start, start + 2};
      else if (start + 3 <= buffer_len)
      {
        if (((
             static_cast<unsigned char>(buffer[start    ]) == 0x9A
          && static_cast<unsigned char>(buffer[start + 1]) >= 0xA0
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xDF
        ) || (
             static_cast<unsigned char>(buffer[start    ]) == 0x9B
          && static_cast<unsigned char>(buffer[start + 1]) >= 0xE0
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xEF
        ) || (
             static_cast<unsigned char>(buffer[start    ]) >= 0x90
          && static_cast<unsigned char>(buffer[start    ]) <= 0x99
          && static_cast<unsigned char>(buffer[start + 1]) >= 0xA0
          // && static_cast<unsigned char>(buffer[start + 1]) <= 0xFF
        )) && (
             static_cast<unsigned char>(buffer[start + 2]) >= 0xA0
          // && static_cast<unsigned char>(buffer[start + 2]) <= 0xFF
        ))
          return {start, start + 3};
        else if (start + 4 <= buffer_len)
        {
          if (((
               static_cast<unsigned char>(buffer[start    ]) == 0x9C
            && static_cast<unsigned char>(buffer[start + 1]) >= 0xF0
            && static_cast<unsigned char>(buffer[start + 1]) <= 0xF4
          ) || (
               static_cast<unsigned char>(buffer[start    ]) == 0x9D
            && static_cast<unsigned char>(buffer[start + 1]) >= 0xF5
            && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
          )) && (
               static_cast<unsigned char>(buffer[start + 2]) >= 0xA0
            // && static_cast<unsigned char>(buffer[start + 2]) <= 0xFF
            && static_cast<unsigned char>(buffer[start + 4]) >= 0xA0
            // && static_cast<unsigned char>(buffer[start + 4]) <= 0xFF
          ))
            return {start, start + 4};
          else
            throw_for_encoding_error("MULE_INTERNAL", buffer, start, 4);
        }
        else
          throw_for_encoding_error("MULE_INTERNAL", buffer, start, 3);
      }
      else
        throw_for_encoding_error("MULE_INTERNAL", buffer, start, 2);
    }
    else
      throw_for_encoding_error("MULE_INTERNAL", buffer, start, 1);
  }
}

template<> seq_position next_seq<encoding_group::SJIS>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  return next_seq_for_sjislike(buffer, buffer_len, start, "SJIS");
}

template<> seq_position next_seq<encoding_group::SHIFT_JIS_2004>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  return next_seq_for_sjislike(buffer, buffer_len, start, "SHIFT_JIS_2004");
}

// https://en.wikipedia.org/wiki/Unified_Hangul_Code
template<> seq_position next_seq<encoding_group::UHC>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) >= 0x80
      && static_cast<unsigned char>(buffer[start]) <= 0xC6
    )
    {
      if (start + 2 <= buffer_len)
      {
        if ((
             static_cast<unsigned char>(buffer[start + 1]) >= 0x41
          && static_cast<unsigned char>(buffer[start + 1]) <= 0x5A
        ) || (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x61
          && static_cast<unsigned char>(buffer[start + 1]) <= 0x7A
        ) || (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
        ))
          return {start, start + 2};
        else
          throw_for_encoding_error("UHC", buffer, start, 2);
      }
      else
        throw_for_encoding_error(
          "UHC",
          buffer,
          start,
          buffer_len - start
        );
    }
    else if (
         static_cast<unsigned char>(buffer[start]) >= 0xA1
      && static_cast<unsigned char>(buffer[start]) <= 0xFE
    )
    {
      if (start + 2 <= buffer_len)
      {
        if (
             static_cast<unsigned char>(buffer[start + 1]) >= 0xA1
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xFE
        )
          return {start, start + 2};
        else
          throw_for_encoding_error("UHC", buffer, start, 2);
      }
      else
        throw_for_encoding_error(
          "UHC",
          buffer,
          start,
          buffer_len - start
        );
    }
    else
      throw_for_encoding_error("UHC", buffer, start, 1);
  }
}

// https://en.wikipedia.org/wiki/UTF-8#Description
template<> seq_position next_seq<encoding_group::UTF8>(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start]) < 0x80)
    return {start, start + 1};
  else
  {
    if (
         static_cast<unsigned char>(buffer[start]) >= 0xC0
      && static_cast<unsigned char>(buffer[start]) <= 0xDF
    )
    {
      if (start + 2 <= buffer_len)
      {
        if (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xBF
        )
          return {start, start + 2};
        else
          throw_for_encoding_error("UTF8", buffer, start, 2);
      }
      else
        throw_for_encoding_error(
          "UTF8",
          buffer,
          start,
          buffer_len - start
        );
    }
    else if (
         static_cast<unsigned char>(buffer[start]) >= 0xE0
      && static_cast<unsigned char>(buffer[start]) <= 0xEF
    )
    {
      if (start + 3 <= buffer_len)
      {
        if (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xBF
          && static_cast<unsigned char>(buffer[start + 2]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 2]) <= 0xBF
        )
          return {start, start + 3};
        else
          throw_for_encoding_error("UTF8", buffer, start, 3);
      }
      else
        throw_for_encoding_error(
          "UTF8",
          buffer,
          start,
          buffer_len - start
        );
    }
    else if (
         static_cast<unsigned char>(buffer[start]) >= 0xF0
      && static_cast<unsigned char>(buffer[start]) <= 0xF7
    )
    {
      if (start + 4 <= buffer_len)
      {
        if (
             static_cast<unsigned char>(buffer[start + 1]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 1]) <= 0xBF
          && static_cast<unsigned char>(buffer[start + 2]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 2]) <= 0xBF
          && static_cast<unsigned char>(buffer[start + 3]) >= 0x80
          && static_cast<unsigned char>(buffer[start + 3]) <= 0xBF
        )
          return {start, start + 4};
        else
          throw_for_encoding_error("UTF8", buffer, start, 4);
      }
      else
        throw_for_encoding_error(
          "UTF8",
          buffer,
          start,
          buffer_len - start
        );
    }
    else
      throw_for_encoding_error("UTF8", buffer, start, 1);
  }
}

} // namespace pqxx::internal
} // namespace pqxx


namespace pqxx
{
namespace internal
{

encoding_group enc_group(int libpq_enc_id)
{
  return enc_group(pg_encoding_to_char(libpq_enc_id));
}

encoding_group enc_group(const std::string& encoding_name)
{
  static const std::map<std::string, encoding_group> encoding_map{
    {"BIG5", encoding_group::BIG5},
    {"EUC_CN", encoding_group::EUC_CN},
    {"EUC_JP", encoding_group::EUC_JP},
    {"EUC_JIS_2004", encoding_group::EUC_JIS_2004},
    {"EUC_KR", encoding_group::EUC_KR},
    {"EUC_TW", encoding_group::EUC_TW},
    {"GB18030", encoding_group::GB18030},
    {"GBK", encoding_group::GBK},
    {"ISO_8859_5", encoding_group::MONOBYTE},
    {"ISO_8859_6", encoding_group::MONOBYTE},
    {"ISO_8859_7", encoding_group::MONOBYTE},
    {"ISO_8859_8", encoding_group::MONOBYTE},
    {"JOHAB", encoding_group::JOHAB},
    {"KOI8R", encoding_group::MONOBYTE},
    {"KOI8U", encoding_group::MONOBYTE},
    {"LATIN1", encoding_group::MONOBYTE},
    {"LATIN2", encoding_group::MONOBYTE},
    {"LATIN3", encoding_group::MONOBYTE},
    {"LATIN4", encoding_group::MONOBYTE},
    {"LATIN5", encoding_group::MONOBYTE},
    {"LATIN6", encoding_group::MONOBYTE},
    {"LATIN7", encoding_group::MONOBYTE},
    {"LATIN8", encoding_group::MONOBYTE},
    {"LATIN9", encoding_group::MONOBYTE},
    {"LATIN10", encoding_group::MONOBYTE},
    {"MULE_INTERNAL", encoding_group::MULE_INTERNAL},
    {"SJIS", encoding_group::SJIS},
    {"SHIFT_JIS_2004", encoding_group::SHIFT_JIS_2004},
    {"SQL_ASCII", encoding_group::MONOBYTE},
    {"UHC", encoding_group::UHC},
    {"UTF8", encoding_group::UTF8},
    {"WIN866", encoding_group::MONOBYTE},
    {"WIN874", encoding_group::MONOBYTE},
    {"WIN1250", encoding_group::MONOBYTE},
    {"WIN1251", encoding_group::MONOBYTE},
    {"WIN1252", encoding_group::MONOBYTE},
    {"WIN1253", encoding_group::MONOBYTE},
    {"WIN1254", encoding_group::MONOBYTE},
    {"WIN1255", encoding_group::MONOBYTE},
    {"WIN1256", encoding_group::MONOBYTE},
    {"WIN1257", encoding_group::MONOBYTE},
    {"WIN1258", encoding_group::MONOBYTE},
  };
  
  auto found_encoding_group{encoding_map.find(encoding_name)};
  if (found_encoding_group == encoding_map.end())
    throw std::invalid_argument{
      "unrecognized encoding '" + encoding_name + "'"
    };
  return found_encoding_group->second;
}

// Utility macro for implementing rutime-switched versions of templated encoding
// functions
#define DISPATCH_ENCODING_OPERATION(ENC, FUNCTION, ARGS...) \
switch (ENC) \
{ \
case encoding_group::MONOBYTE: \
  return FUNCTION<encoding_group::MONOBYTE>(ARGS); \
case encoding_group::BIG5: \
  return FUNCTION<encoding_group::BIG5>(ARGS); \
case encoding_group::EUC_CN: \
  return FUNCTION<encoding_group::EUC_CN>(ARGS); \
case encoding_group::EUC_JP: \
  return FUNCTION<encoding_group::EUC_JP>(ARGS); \
case encoding_group::EUC_JIS_2004: \
  return FUNCTION<encoding_group::EUC_JIS_2004>(ARGS); \
case encoding_group::EUC_KR: \
  return FUNCTION<encoding_group::EUC_KR>(ARGS); \
case encoding_group::EUC_TW: \
  return FUNCTION<encoding_group::EUC_TW>(ARGS); \
case encoding_group::GB18030: \
  return FUNCTION<encoding_group::GB18030>(ARGS); \
case encoding_group::GBK: \
  return FUNCTION<encoding_group::GBK>(ARGS); \
case encoding_group::JOHAB: \
  return FUNCTION<encoding_group::JOHAB>(ARGS); \
case encoding_group::MULE_INTERNAL: \
  return FUNCTION<encoding_group::MULE_INTERNAL>(ARGS); \
case encoding_group::SJIS: \
  return FUNCTION<encoding_group::SJIS>(ARGS); \
case encoding_group::SHIFT_JIS_2004: \
  return FUNCTION<encoding_group::SHIFT_JIS_2004>(ARGS); \
case encoding_group::UHC: \
  return FUNCTION<encoding_group::UHC>(ARGS); \
case encoding_group::UTF8: \
  return FUNCTION<encoding_group::UTF8>(ARGS); \
} \
throw pqxx::usage_error("Invalid encoding group code.")

seq_position next_seq(
  encoding_group enc,
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  DISPATCH_ENCODING_OPERATION(enc, next_seq, buffer, buffer_len, start);
}

std::string::size_type find_with_encoding(
  encoding_group enc,
  const std::string& haystack,
  char needle,
  std::string::size_type start
)
{
  DISPATCH_ENCODING_OPERATION(enc, find_with_encoding, haystack, needle, start);
}

std::string::size_type find_with_encoding(
  encoding_group enc,
  const std::string& haystack,
  const std::string& needle,
  std::string::size_type start
)
{
  DISPATCH_ENCODING_OPERATION(enc, find_with_encoding, haystack, needle, start);
}

#undef DISPATCH_ENCODING_OPERATION

} // namespace pqxx::internal
} // namespace pqxx
