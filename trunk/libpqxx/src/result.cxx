/*-------------------------------------------------------------------------
 *
 *   FILE
 *	result.cxx
 *
 *   DESCRIPTION
 *      implementation of the pqxx::Result class and support classes.
 *   pqxx::Result represents the set of result tuples from a database query
 *
 * Copyright (c) 2001-2002, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 *-------------------------------------------------------------------------
 */
#include <new>
#include <stdexcept>

#include "pqxx/result.h"


using namespace PGSTD;


pqxx::Result &pqxx::Result::operator=(const pqxx::Result &Other)
{
  if (Other.m_Result != m_Result)
  {
    LoseRef();
    MakeRef(Other);
  }
  return *this;
}


pqxx::Result &pqxx::Result::operator=(PGresult *Other)
{
  if (Other != m_Result)
  {
    LoseRef();
    MakeRef(Other);
  }
  return *this;
}



const pqxx::Result::Tuple pqxx::Result::at(pqxx::Result::size_type i) const
{
  if ((i < 0) || (i >= size()))
    throw out_of_range("Tuple number out of range");

  return operator[](i);
}


void pqxx::Result::CheckStatus() const
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
		      "pqxx::Result: Unrecognized response code " +
		      ToString(int(PQresultStatus(m_Result))));
  }
}


void pqxx::Result::MakeRef(PGresult *Other)
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


void pqxx::Result::MakeRef(const pqxx::Result &Other)
{
  m_Result = Other.m_Result;
  m_Refcount = Other.m_Refcount;
  if (Other.m_Refcount)
    ++*m_Refcount;
}


void pqxx::Result::LoseRef() throw ()
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


const char *pqxx::Result::GetValue(pqxx::Result::size_type Row, 
		                 pqxx::Result::Tuple::size_type Col) const
{
  return PQgetvalue(m_Result, Row, Col);
}


bool pqxx::Result::GetIsNull(pqxx::Result::size_type Row,
		           pqxx::Result::Tuple::size_type Col) const
{
  return PQgetisnull(m_Result, Row, Col) != 0;
}

pqxx::Result::Field::size_type 
pqxx::Result::GetLength(pqxx::Result::size_type Row,
                        pqxx::Result::Tuple::size_type Col) const
{
  return PQgetlength(m_Result, Row, Col);
}



// Tuple

pqxx::Result::Field pqxx::Result::Tuple::operator[](const char f[]) const
{
  return Field(*this, m_Home->ColumnNumber(f));
}


pqxx::Result::Field pqxx::Result::Tuple::at(const char f[]) const
{
  const int fnum = m_Home->ColumnNumber(f);
  // TODO: Build check into ColumnNumber()
  if (fnum == -1)
    throw invalid_argument(string("Unknown field '") + f + "'");

  return Field(*this, fnum);
}


pqxx::Result::Field pqxx::Result::Tuple::at(pqxx::Result::Tuple::size_type i) const
{
  if ((i < 0) || (i >= size()))
    throw out_of_range("Invalid field number");

  return operator[](i);
}



// const_iterator

pqxx::Result::const_iterator pqxx::Result::const_iterator::operator++(int)
{
  const_iterator Old(*this);
  m_Index++;
  return Old;
}


pqxx::Result::const_iterator pqxx::Result::const_iterator::operator--(int)
{
  const_iterator Old(*this);
  m_Index--;
  return Old;
}


