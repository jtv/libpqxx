/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pg_result.cc
 *
 *   DESCRIPTION
 *      implementation of the Pg::Result class and support classes.
 *   Pg::Result represents the set of result tuples from a database query
 *
 * Copyright (c) 2001, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <stdexcept>

#include "pg_result.h"


using namespace PGSTD;


Pg::Result &Pg::Result::operator=(const Pg::Result &Other)
{
  if (Other.m_Result != m_Result)
  {
    LoseRef();
    MakeRef(Other);
  }
  return *this;
}


Pg::Result &Pg::Result::operator=(PGresult *Other)
{
  if (Other != m_Result)
  {
    LoseRef();
    MakeRef(Other);
  }
  return *this;
}



const Pg::Result::Tuple Pg::Result::at(Pg::Result::size_type i) const
{
  if ((i < 0) || (i >= size()))
    throw out_of_range("Tuple number out of range");

  return operator[](i);
}


void Pg::Result::CheckStatus() const
{
  if (!m_Result)
    throw runtime_error("No result");

  switch (PQresultStatus(m_Result))
  {
  case PGRES_EMPTY_QUERY: // The string sent to the backend was empty.
  case PGRES_COMMAND_OK: // Successful completion of a command returning no data
  case PGRES_TUPLES_OK: // The query successfully executed
    break;

  case PGRES_COPY_OUT: // Copy Out (from server) data transfer started
  case PGRES_COPY_IN: // Copy In (to server) data transfer started
    break;

  case PGRES_BAD_RESPONSE: // The server's response was not understood
  case PGRES_NONFATAL_ERROR: // TODO: Is this one really an error?
  case PGRES_FATAL_ERROR:
    throw runtime_error(PQresultErrorMessage(m_Result));

  default:
    throw logic_error("Internal libpqxx error: "
		      "Pg::Result: Unrecognized response code " +
		      ToString(int(PQresultStatus(m_Result))));
  }
}


void Pg::Result::MakeRef(PGresult *Other)
{
  if (Other)
  {
    try
    {
      m_Refcount = new int(1);
    }
    catch (const bad_alloc &)
    {
      PQclear(Other);
      throw;
    }
  }
  m_Result = Other;
}


void Pg::Result::MakeRef(const Pg::Result &Other)
{
  m_Result = Other.m_Result;
  m_Refcount = Other.m_Refcount;
  if (Other.m_Refcount)
    ++*m_Refcount;
}


void Pg::Result::LoseRef() throw ()
{
  if (m_Refcount)
  {
    --*m_Refcount;
    if (*m_Refcount <= 0)
    {
      delete m_Refcount;
      m_Refcount = 0;
    }
  }

  if (!m_Refcount && m_Result)
  {
    PQclear(m_Result);
    m_Result = 0;
  }
}



// Tuple

Pg::Result::Field Pg::Result::Tuple::operator[](const char f[]) const
{
  return Field(*this, PQfnumber(Result_c_ptr(), f));
}


Pg::Result::Field Pg::Result::Tuple::at(const char f[]) const
{
  const int fnum = PQfnumber(Result_c_ptr(), f);
  if (fnum == -1)
    throw invalid_argument(string("Unknown field '") + f + "'");

  return Field(*this, fnum);
}


Pg::Result::Field Pg::Result::Tuple::at(Pg::Result::Tuple::size_type i) const
{
  if ((i < 0) || (i >= size()))
    throw out_of_range("Invalid field number");

  return operator[](i);
}



// Field

const char *Pg::Result::Field::c_str() const
{
  return PQgetvalue(Result_c_ptr(), m_Index, m_Col);
}


const char *Pg::Result::Field::name() const
{
  return PQfname(Result_c_ptr(), m_Col);
}


int Pg::Result::Field::size() const
{
  return PQgetlength(Result_c_ptr(), m_Index, m_Col);
}


bool Pg::Result::Field::is_null() const
{
  return PQgetisnull(Result_c_ptr(), m_Index, m_Col);
}


// const_iterator

Pg::Result::const_iterator Pg::Result::const_iterator::operator++(int)
{
  const_iterator Old(*this);
  m_Index++;
  return Old;
}


Pg::Result::const_iterator Pg::Result::const_iterator::operator--(int)
{
  const_iterator Old(*this);
  m_Index--;
  return Old;
}


