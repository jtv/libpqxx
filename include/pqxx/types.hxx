/**
 * Basic typedefs and forward declarations.
 *
 * Copyright (c) 2017, Jeroen T. Vermeulen
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this
 * mistake, or contact the author.
 */
#ifndef PQXX_TYPES
#define PQXX_TYPES

namespace pqxx
{
/// Number of rows in a result set.
typedef unsigned long result_size_type;

/// Difference between result sizes.
typedef signed long result_difference_type;

/// Number of fields in a row of database data.
typedef unsigned int row_size_type;

/// Difference between row sizes.
typedef signed int row_difference_type;

/// Number of bytes in a field of database data.
typedef size_t field_size_type;

/// Number of bytes in a large object.  (Unusual: it's signed.)
typedef long large_object_size_type;


// Forward declarations, to help break compilation dependencies.
// These won't necessarily include all classes in libpqxx.
class binarystring;
class connectionpolicy;
class connection_base;
class const_result_iterator;
class const_reverse_result_iterator;
class const_reverse_row_iterator;
class const_row_iterator;
class dbtransaction;
class largeobjectaccess;
class notification_receiver;
class range_error;
class result;
class row;
class tablereader;
class transaction_base;

} // namespace pqxx

#endif
