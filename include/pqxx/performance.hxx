/*-------------------------------------------------------------------------
 *
 *   FILE
 *	pqxx/performance.hxx
 *
 *   DESCRIPTION
 *      performance features documentation
 *   Documentation only: libpqxx performance features.
 *   DO NOT INCLUDE THIS FILE; it's here only to provide documentation.
 *
 * Copyright (c) 2011, Jeroen T. Vermeulen <jtv@xs4all.nl>
 *
 * See COPYING for copyright license.  If you did not receive a file called
 * COPYING with this source code, please notify the distributor of this mistake,
 * or contact the author.
 *
 *-------------------------------------------------------------------------
 */

/**
 * @defgroup performance Performance features
 *
 * If your program's database interaction is not as efficient as it needs to be,
 * the first place to look is usually the SQL you're executing.  But libpqxx
 * has a few specialized features to help you squeeze a bit more performance out
 * of how you issue commands and retrieve data:
 * \li pqxx::pipeline lets you send queries to the database in batch, and
 *     continue other processing while they are executing.
 * \li @ref prepared.  These can be executed many times without the server
 *     parsing and planning them each and every time.  They also save you having
 *     to escape string parameters.
 *
 * As always of course, don't risk the quality of your code for optimizations
 * that you don't need!
 */
