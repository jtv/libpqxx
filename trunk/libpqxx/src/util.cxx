/*-------------------------------------------------------------------------
 *
 *   FILE
 *	util.cxx
 *
 *   DESCRIPTION
 *      Various utility functions for libpqxx
 *
 * Copyright (c) 2003-2004, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */
#include "pqxx/compiler.h"

#include <new>

#include "pqxx/util"

using namespace PGSTD;


void pqxx::internal::FromString_string(const char Str[], string &Obj)
{
  if (!Str)
    throw runtime_error("Attempt to convert NULL C string to C++ string");
  Obj = Str;
}


void pqxx::internal::FromString_ucharptr(const char Str[], 
    const unsigned char *&Obj)
{
  const char *C;
  FromString(Str, C);
  Obj = reinterpret_cast<const unsigned char *>(C);
}


void pqxx::internal::FromString_bool(const char Str[], bool &Obj)
{
  if (!Str)
    throw runtime_error("Attempt to read NULL string");

  bool OK;

  switch (Str[0])
  {
  case 0:
    Obj = false;
    OK = true;
    break;

  case 'f':
  case 'F':
    Obj = false;
    OK = !(Str[1] && 
	   (strcmp(Str+1, "alse") != 0) && 
	   (strcmp(Str+1, "ALSE") != 0));
    break;

  case '0':
    {
      int I;
      FromString(Str, I);
      Obj = (I != 0);
      OK = ((I == 0) || (I == 1));
    }
    break;

  case '1':
    Obj = true;
    OK = !Str[1];
    break;

  case 't':
  case 'T':
    Obj = true;
    OK = !(Str[1] &&
	   (strcmp(Str+1, "rue") != 0) &&
	   (strcmp(Str+1, "RUE") != 0));
    break;

  default:
    OK = false;
  }

  if (!OK) 
    throw invalid_argument("Failed conversion to bool: '" + string(Str) + "'");
}


string pqxx::internal::Quote_string(const string &Obj, bool EmptyIsNull)
{
  if (EmptyIsNull && Obj.empty()) return "null";

  string Result;
  Result.reserve(Obj.size() + 2);
  Result += "'";

#ifdef PQXX_HAVE_PQESCAPESTRING

  char *const Buf = new char[2*Obj.size() + 1];
  try
  {
    PQescapeString(Buf, Obj.c_str(), Obj.size());
    Result += Buf;
  }
  catch (const PGSTD::exception &)
  {
    delete [] Buf;
    throw;
  }
  delete [] Buf;

#else

  for (PGSTD::string::size_type i=0; i < Obj.size(); ++i)
  {
    if (isgraph(Obj[i]))
    {
      switch (Obj[i])
      {
      case '\'':
      case '\\':
	Result += '\\';
      }
      Result += Obj[i];
    }
    else
    {
        char s[10];
        sprintf(s, 
	        "\\%03o", 
		static_cast<unsigned int>(static_cast<unsigned char>(Obj[i])));
        Result.append(s, 4);
    }
  }

#endif

  return Result + '\'';
}


string pqxx::internal::Quote_charptr(const char Obj[], bool EmptyIsNull)
{
  if (!Obj) return "null";
  return Quote(string(Obj), EmptyIsNull);
}


string pqxx::internal::namedclass::description() const throw ()
{
  try
  {
    string desc = classname();
    if (!name().empty()) desc += " '" + name() + "'";
    return desc;
  }
  catch (const exception &)
  {
    // Oops, string composition failed!  Probably out of memory.
    // Let's try something easier.
  }
  return name().empty() ? classname() : name();
}


void pqxx::internal::CheckUniqueRegistration(const namedclass *New,
    const namedclass *Old)
{
  if (!New) 
    throw logic_error("libpqxx internal error: NULL pointer registered");
  if (Old)
  {
    if (Old == New)
      throw logic_error("Started " + New->description() + " twice");
    throw logic_error("Started " + New->description() + " "
		      "while " + Old->description() + " still active");
  }
}


void pqxx::internal::CheckUniqueUnregistration(const namedclass *New,
    const namedclass *Old)
{
  if (New != Old)
  {
    if (!New)
      throw logic_error("Expected to close " + Old->description() + ", "
	  		"but got NULL pointer instead");
    if (!Old)
      throw logic_error("Closed " + New->description() + ", "
	 		"which wasn't open");
    throw logic_error("Closed " + New->description() + "; "
		      "expected to close " + Old->description());
  }
}

