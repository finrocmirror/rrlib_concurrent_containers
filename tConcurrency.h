//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/concurrent_containers/tConcurrency.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-24
 *
 * \brief   Contains tConcurrency
 *
 * \b tConcurrency
 *
 * Possible concurrency settings for concurrent data structures.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tConcurrency_h__
#define __rrlib__concurrent_containers__tConcurrency_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace rrlib
{
namespace concurrent_containers
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Possible concurrency settings
/*!
 * Possible concurrency settings for concurrent data structures.
 */
enum class tConcurrency
{
  NONE,                          //!< Reading and Writing is performed by the same thread (very efficient)
  SINGLE_READER_AND_WRITER,      //!< Only a single thread may write concurrently. Only one (other) thread may read concurrently.
  MULTIPLE_WRITERS,              //!< Multiple threads may write concurrently
  MULTIPLE_READERS,              //!< Multiple threads may read concurrently
  FULL,                          //!< Multiple threads may read and write concurrently
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
