/*-------------------------------------------------------------------------
 *
 *   FILE
 *	util.cxx
 *
 *   DESCRIPTION
 *      Various utility functions for libpqxx
 *
 * Copyright (c) 2003, Jeroen T. Vermeulen <jtv@xs4all.nl>
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


void pqxx::FromString_string(const char Str[], string &Obj)
{
  if (!Str)
    throw runtime_error("Attempt to convert NULL C string to C++ string");
  Obj = Str;
}


void pqxx::FromString_ucharptr(const char Str[], const unsigned char *&Obj)
{
  const char *C;
  FromString(Str, C);
  Obj = reinterpret_cast<const unsigned char *>(C);
}


void pqxx::FromString_bool(const char Str[], bool &Obj)
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

  default:
    OK = false;
  }

  if (!OK) 
    throw invalid_argument("Failed conversion to bool: '" + string(Str) + "'");
}


string pqxx::Quote_string(const string &Obj, bool EmptyIsNull)
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


string pqxx::Quote_charptr(const char Obj[], bool EmptyIsNull)
{
  if (!Obj) return "null";
  return Quote(string(Obj), EmptyIsNull);
}


string pqxx::UniqueRegisterError(const void *New,
    				 const void *Old,
				 const string &ClassName,
				 const string &NewName)
{
  string Result;
  if (!New) 
    Result = "libpqxx internal error: NULL " + ClassName;
  else if (!Old)
    throw logic_error("libpqxx internal error: unique<" + 
	ClassName + "> error, but both old and new pointers are valid");
  else if (Old == New)
    Result = ClassName + " '" + NewName + "' "
	"started more than once without closing";
  else
    Result = "Started " + ClassName + " '" + NewName + "' "
	"while '" + NewName + "' was still active";

  return Result;
}


string pqxx::UniqueUnregisterError(const void *New,
    				   const void *Old,
				   const string &ClassName,
				   const string &NewName,
				   const string &OldName)
{
  string Result;
  if (!New) 
    Result = "Closing NULL " + ClassName;
  else if (!Old)
    Result = "Closing " + ClassName + " '" + NewName + "' "
    	"which wasn't open";
  else
    Result = "Closing wrong " + ClassName + "; expect '" + OldName + "' "
    	"but got '" + NewName + "'";
  return Result;
}


