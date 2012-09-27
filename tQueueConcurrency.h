//
// You received this file as part of RRLib
// Robotics Research Library
//
// Copyright (C) Finroc GbR (finroc.org)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
//----------------------------------------------------------------------
/*!\file    rrlib/concurrent_containers/tQueueConcurrency.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-24
 *
 * \brief   Contains tQueueConcurrency
 *
 * \b tQueueConcurrency
 *
 * Possible concurrency settings for queues.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tQueueConcurrency_h__
#define __rrlib__concurrent_containers__tQueueConcurrency_h__

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
//! Possible concurrency settings for queues.
/*!
 * Possible concurrency settings for queues.
 *
 * In settings postfixed with _FAST, the last element in the queue is not dequeueable -
 * so the queue always contains at least one element.
 * Such queues are more efficient with respect to computational overhead.
 */
enum class tQueueConcurrency
{
  NONE,                          //!< Enqueueing and Dequeueing is performed by the same thread (very efficient)
  //SINGLE_READER_AND_WRITER,    //!< Only a single thread may enqueue concurrently. Only one (other) thread may dequeue concurrently.
  SINGLE_READER_AND_WRITER_FAST, //!< Only a single thread may enqueue concurrently. Only one (other) thread may dequeue concurrently.
  MULTIPLE_WRITERS,              //!< Multiple threads may enqueue concurrently
  MULTIPLE_WRITERS_FAST,         //!< Multiple threads may enqueue concurrently
  MULTIPLE_READERS_FAST,         //!< Multiple threads may dequeue concurrently
  FULL_FAST,                     //!< Mutliple threads may enqueue and dequeue concurrently
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
