/** Implementation of string encodings support
 *
 * Copyright (c) 2001-2018, Jeroen T. Vermeulen.
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 */
#include "pqxx/compiler-internal.hxx"

#include "pqxx/internal/encodings.hxx"

#include <exception>
#include <iomanip>
#include <sstream>

using namespace pqxx::internal;


#define THROW_FOR_ENCODING_ERROR(ENCODING_NAME, BUFFER, START, COUNT) \
{ \
    std::stringstream s; \
    s \
        << "invalid sequence " \
        << std::hex \
        << std::setw(2) \
        << std::setfill('0') \
    ; \
    for (std::size_t i{0}; i < COUNT; ++i) \
        s \
            << "0x" \
            << static_cast<unsigned int>( \
                static_cast<unsigned char>(BUFFER[START + i]) \
            ) \
            << " " \
        ; \
    s \
        << "at byte " \
        << std::dec \
        << std::setw(0) \
        << START \
        << " for encoding " \
        << ENCODING_NAME \
    ; \
    throw std::runtime_error{s.str()}; \
}


namespace
{

seq_position next_char_monobyte(
  const char* buffer,
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
seq_position next_char_BIG5(
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
        THROW_FOR_ENCODING_ERROR("BIG5", buffer, start, 2)
    }
    else
      THROW_FOR_ENCODING_ERROR("BIG5", buffer, start, 1)
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
seq_position next_char_EUC_CN(
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
        THROW_FOR_ENCODING_ERROR("EUC_CN", buffer, start, 2)
    }
    else
      THROW_FOR_ENCODING_ERROR("EUC_CN", buffer, start, 1)
  }
}

/*
EUC-JP and EUC-JIS-2004 represent slightly different code points but iterate
the same
https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-JP
http://x0213.org/codetable/index.en.html
*/
seq_position next_char_for_euc_jplike(
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start,
  const char* encoding_name
)
{
  if (start >= buffer_len)
    return {std::string::npos, std::string::npos};
  else if (static_cast<unsigned char>(buffer[start] ) < 0x80)
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
        THROW_FOR_ENCODING_ERROR(encoding_name, buffer, start, 2)
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
        THROW_FOR_ENCODING_ERROR(encoding_name, buffer, start, 2)
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
        THROW_FOR_ENCODING_ERROR(encoding_name, buffer, start, 3)
    }
    else
      THROW_FOR_ENCODING_ERROR(encoding_name, buffer, start, 1)
  }
}

// https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-KR
seq_position next_char_EUC_KR(
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
        THROW_FOR_ENCODING_ERROR("EUC_KR", buffer, start, 1)
    }
    else
      THROW_FOR_ENCODING_ERROR("EUC_KR", buffer, start, 1)
  }
}

// https://en.wikipedia.org/wiki/Extended_Unix_Code#EUC-TW
seq_position next_char_EUC_TW(
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
        THROW_FOR_ENCODING_ERROR("EUC_KR", buffer, start, 2)
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
        THROW_FOR_ENCODING_ERROR("EUC_KR", buffer, start, 4)
    }
    else
      THROW_FOR_ENCODING_ERROR("EUC_KR", buffer, start, 1)
  }
}

// https://en.wikipedia.org/wiki/GB_18030#Mapping
seq_position next_char_GB18030(
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
        THROW_FOR_ENCODING_ERROR("GB18030", buffer, start, 2)
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
          THROW_FOR_ENCODING_ERROR("GB18030", buffer, start, 4)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          "GB18030",
          buffer,
          start,
          buffer_len - start
        )
    }
  }
}

// https://en.wikipedia.org/wiki/GBK_(character_encoding)#Encoding
seq_position next_char_GBK(
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
        THROW_FOR_ENCODING_ERROR("GBK", buffer, start, 2)
    }
    else
      THROW_FOR_ENCODING_ERROR("GBK", buffer, start, 1)
  }
}

/*
The PostgreSQL documentation claims that the JOHAB encoding is 1-3 bytes, but
"CJKV Information Processing" describes it (actually just the Hangul portion) as
"three five-bit segments" that reside inside 16 bits (2 bytes).
*/
// CJKV Information Processing by Ken Lunde, pg. 269
// https://books.google.com/books?id=SA92uQqTB-AC&pg=PA269&lpg=PA269&dq=JOHAB+encoding&source=bl&ots=GMvxWWl8Gx&sig=qLFQNkR4d7Rd-iqQy1lNh3oEdOE&hl=en&sa=X&ved=0ahUKEwizyoTDxePbAhWjpFkKHU65DSwQ6AEIajAH#v=onepage&q=JOHAB%20encoding&f=false
seq_position next_char_JOHAB(
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
        THROW_FOR_ENCODING_ERROR("JOHAB", buffer, start, 2)
    }
    else
      THROW_FOR_ENCODING_ERROR("JOHAB", buffer, start, 1)
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
seq_position next_char_sjis_like(
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
          THROW_FOR_ENCODING_ERROR(encoding_name, buffer, start, 2)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          encoding_name,
          buffer,
          start,
          buffer_len - start
        )
    }
    else
      THROW_FOR_ENCODING_ERROR(encoding_name, buffer, start, 1)
  }
}

// https://en.wikipedia.org/wiki/Unified_Hangul_Code
seq_position next_char_UHC(
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
          THROW_FOR_ENCODING_ERROR("UHC", buffer, start, 2)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          "UHC",
          buffer,
          start,
          buffer_len - start
        )
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
          THROW_FOR_ENCODING_ERROR("UHC", buffer, start, 2)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          "UHC",
          buffer,
          start,
          buffer_len - start
        )
    }
    else
      THROW_FOR_ENCODING_ERROR("UHC", buffer, start, 1)
  }
}

// https://en.wikipedia.org/wiki/UTF-8#Description
seq_position next_char_UTF8(
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
          THROW_FOR_ENCODING_ERROR("UTF8", buffer, start, 2)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          "UTF8",
          buffer,
          start,
          buffer_len - start
        )
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
          THROW_FOR_ENCODING_ERROR("UTF8", buffer, start, 3)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          "UTF8",
          buffer,
          start,
          buffer_len - start
        )
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
          THROW_FOR_ENCODING_ERROR("UTF8", buffer, start, 4)
      }
      else
        THROW_FOR_ENCODING_ERROR(
          "UTF8",
          buffer,
          start,
          buffer_len - start
        )
    }
    else
      THROW_FOR_ENCODING_ERROR("UTF8", buffer, start, 1)
  }
}

} // namespace


#undef THROW_FOR_ENCODING_ERROR


seq_position next_seq(
  encoding enc,
  const char* buffer,
  std::string::size_type buffer_len,
  std::string::size_type start
)
{
  switch (enc)
  {
  // Single-byte encodings

  case encoding::ISO_8859_5:
  case encoding::ISO_8859_6:
  case encoding::ISO_8859_7:
  case encoding::ISO_8859_8:
  case encoding::KOI8R:
  case encoding::KOI8U:
  case encoding::LATIN1:
  case encoding::LATIN2:
  case encoding::LATIN3:
  case encoding::LATIN4:
  case encoding::LATIN5:
  case encoding::LATIN6:
  case encoding::LATIN7:
  case encoding::LATIN8:
  case encoding::LATIN9:
  case encoding::LATIN10:
  case encoding::SQL_ASCII:
  case encoding::WIN866:
  case encoding::WIN874:
  case encoding::WIN1250:
  case encoding::WIN1251:
  case encoding::WIN1252:
  case encoding::WIN1253:
  case encoding::WIN1254:
  case encoding::WIN1255:
  case encoding::WIN1256:
  case encoding::WIN1257:
  case encoding::WIN1258:
    return next_char_monobyte(buffer, buffer_len, start);

  // Multi-byte encodings

  case encoding::EUC_JP:
  case encoding::EUC_JIS_2004:
    return next_char_for_euc_jplike(
      buffer,
      buffer_len,
      start,
      (enc == encoding::EUC_JP ? "EUC_JP" : "EUC_JIS_2004")
    );
  case encoding::SJIS:
  case encoding::SHIFT_JIS_2004:
    return next_char_sjis_like(
      buffer,
      buffer_len,
      start,
      (enc == encoding::SJIS ? "SJIS" : "SHIFT_JIS_2004")
    );

  case encoding::BIG5:
    return next_char_BIG5(buffer, buffer_len, start);
  case encoding::EUC_CN:
    return next_char_EUC_CN(buffer, buffer_len, start);
  case encoding::EUC_KR:
    return next_char_EUC_KR(buffer, buffer_len, start);
  case encoding::EUC_TW:
    return next_char_EUC_TW(buffer, buffer_len, start);
  case encoding::GB18030:
    return next_char_GB18030(buffer, buffer_len, start);
  case encoding::GBK:
    return next_char_GBK(buffer, buffer_len, start);
  case encoding::JOHAB:
    return next_char_JOHAB(buffer, buffer_len, start);
  case encoding::UHC:
    return next_char_UHC(buffer, buffer_len, start);
  case encoding::UTF8:
    return next_char_UTF8(buffer, buffer_len, start);
  }
}
