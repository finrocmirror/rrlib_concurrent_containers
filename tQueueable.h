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
/*!\file    rrlib/concurrent_containers/tQueueable.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-24
 *
 * \brief   Contains tQueueable
 *
 * \b tQueueable
 *
 * This is the base class of queueable objects that are to be used in intrusive queues.
 * It typically contains the pointer to the next element in a singly-linked queue.
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tQueueable_h__
#define __rrlib__concurrent_containers__tQueueable_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/util/tNoncopyable.h"
#include <atomic>

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

/*!
 * Determines in which queues queueable element can be used.
 * Can also make a difference with respect to computational overhead
 * and memory consumption.
 */
enum class tQueueability
{
  /*!
   * Object can be used in single-threaded only - with much lower
   * computational overhead than using an ordinary queueable object
   * (has size of 1 pointer)
   */
  SINGLE_THREADED,

  /*!
   * Object can be used in most queues.
   * Currently, this excludes concurrent, bounded queues with tDequeueMode::ALL.
   * (has size of 1 pointer)
   */
  MOST,

  /*!
   * Object can be used in most queues.
   * Currently, this excludes concurrent, bounded queues with tDequeueMode::ALL.
   * It has an additional single-threaded pointer that leads to higher
   * computational efficiency in single-threaded queues and queue fragments
   * (has size of 2 pointers (MOST + SINGLE_THREADED)
   */
  MOST_OPTIMIZED,

  /*!
   * Object can be used in all queues
   * (has size of 2 pointers)
   */
  FULL,

  /*!
   * Object can be used in all queues.
   * It has an additional single-threaded pointer that leads to higher
   * computational efficiency in single-threaded queues and queue fragments
   * (has size of 3 pointers (FULL + SINGLE_THREADED)
   */
  FULL_OPTIMIZED
};


//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Base class for queueable object
/*!
 * This is the base class of queueable objects that are to be used in intrusive queues.
 * It typically contains the pointer to the next element in a singly-linked queue.
 *
 * Any classes T derived from this class, can be used very efficiently
 * in queues of type tQueue<std::unique_ptr<T>, ...> - as they
 * are enqueued directly and do not need to be placed in nodes.
 *
 * The public public interface is empty.
 * (internals are not be used outside of queue implementations)
 */
template <tQueueability QUEUEABILITY>
class tQueueable : private rrlib::util::tNoncopyable
{
};


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

#include "rrlib/concurrent_containers/queue/tQueueableImplementation.h"

#endif
