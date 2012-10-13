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
 * This is the base class of queueable objects.
 * It contains the pointer to the next element in singly-linked queue.
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tQueueable_h__
#define __rrlib__concurrent_containers__tQueueable_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <boost/utility.hpp>
#include <atomic>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueConcurrency.h"

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
namespace queue
{
template <typename T, typename D, tQueueConcurrency CONCURRENCY, bool BOUNDED, bool QUEUEABLE_TYPE, bool QUEUEABLE_SINGLE_THREADED_TYPE>
class tUniquePtrQueueImplementation;

template <typename T, typename D, bool CONCURRENT>
class tFastUniquePtrQueueEnqueueImplementation;

template <typename T, typename D, bool CONCURRENT_ENQUEUE, bool CONCURRENT_DEQUEUE>
class tFastUniquePtrQueueDequeueImplementation;

template <typename T, typename D, bool FAST>
class tUniquePtrBoundedDequeueImplementation;

template <typename T, typename D, bool CONCURRENT, bool FAST>
class tUniquePtrBoundedEnqueueImplementation;
}

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Base class for queueable object
/*!
 * This is the base class of queueable objects.
 * It contains the pointer to the next element in a singly-linked queue.
 *
 * Any classes T derived from this class, can be used very efficiently
 * in queues of type tQueue<std::unique_ptr<T>, CONCURRENCY> - as they
 * are enqueued directly and do not need to be placed in nodes.
 *
 * If objects are used in single-threaded queues only, you should consider
 * deriving from tQueueableSingleThreaded instead - as this is more efficient.
 *
 * If objects are used in single-threaded queues also, you should consider
 * deriving from tQueueableSingleThreaded as well - as this is more efficient
 * (computationally, object size will slightly increase though).
 */
class tQueueable : boost::noncopyable
{
//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tQueueable();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  template <typename T, typename D, tQueueConcurrency CONCURRENCY, bool BOUNDED, bool QUEUEABLE_TYPE, bool QUEUEABLE_SINGLE_THREADED_TYPE>
  friend class queue::tUniquePtrQueueImplementation;

  template <typename T, typename D, bool CONCURRENT>
  friend class queue::tFastUniquePtrQueueEnqueueImplementation;

  template <typename T, typename D, bool CONCURRENT_ENQUEUE, bool CONCURRENT_DEQUEUE>
  friend class queue::tFastUniquePtrQueueDequeueImplementation;

  template <typename T, typename D, bool FAST>
  friend class queue::tUniquePtrBoundedDequeueImplementation;

  template <typename T, typename D, bool CONCURRENT, bool FAST>
  friend class queue::tUniquePtrBoundedEnqueueImplementation;

  tQueueable(bool terminator);

  /*!
   * Pointer to next element in reuse queue... null if there's none
   *
   * Needs to be atomic - anything else would not really be clean
   * (unfortunately, this really hurts performance - almost factor 10).
   * Queue stress test fails with non-atomic pointer with multiple writer threads.
   */
  std::atomic<tQueueable*> next_queueable;

  /*! Terminator (not null for efficiency reasons) */
  static tQueueable terminator;
};


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
