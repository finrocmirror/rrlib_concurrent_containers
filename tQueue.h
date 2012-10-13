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
/*!\file    rrlib/concurrent_containers/tQueue.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-23
 *
 * \brief   Contains tQueue
 *
 * \b tQueue
 *
 * Concurrent non-blocking FIFO linked Queue.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tQueue_h__
#define __rrlib__concurrent_containers__tQueue_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueConcurrency.h"
#include "rrlib/concurrent_containers/tQueueable.h"
#include "rrlib/concurrent_containers/queue/tQueueImplementation.h"

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
//! Concurrent non-blocking FIFO linked Queue.
/*!
 * This is a concurrent non-blocking FIFO linked queue.
 * It should be suitable for real-time code, as it does not need to allocate memory.
 *
 * Depending on the template parameters, it allows concurrent enqueueing
 * and dequeueing operations.
 * There is no size limit.
 *
 * Using this queue is most efficient, when using std::unique_ptr<U> as type T, with U
 * derived from tQueueable (optionally tQueueableSingleThreaded in non-concurrent queues).
 * Otherwise, objects are placed in queue nodes that need to be managed separately.
 * Due to the use of universal queue node objects, sizeof(T) must not be larger than sizeof(void*).
 *
 * \tparam T Enqueued elements. Ideally, std::unique_ptr<U> with with U derived from tQueueable (or tQueueableSingleThreaded).
 * \tparam CONCURRENCY Concurrency that queue should support.
 * \tparam BOUNDED If true, a 'guiding value' for maximum queue length can be specified.
 *                 It can be changed at runtime (up to 500K elements).
 *                 If this length is exceeded, elements enqueued first are discarded.
 *                 Due to concurrency, however, the queue may temporarily contain more elements.
 *                 It is guaranteed that elements are discarded only if the queue length exceeds the specified 'guiding value'.
 */
template <typename T, tQueueConcurrency CONCURRENCY, bool BOUNDED = false>
class tQueue : queue::tQueueImplementation<T, CONCURRENCY, BOUNDED>
{
  typedef queue::tQueueImplementation<T, CONCURRENCY, BOUNDED> tImplementation;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*!
   * Minimum number of elements in queue.
   * In '_FAST' queue implementations this is typically one - meaning that the last
   * element cannot be dequeued (it can, as soon as another element is enqueued).
   * Otherwise it's zero.
   */
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = tImplementation::cMINIMUM_ELEMENTS_IN_QEUEUE };

  /*!
   * Remove first element from queue and return it.
   * May only be called by a multiple reader threads concurrently, if CONCURRENT_DEQUEUEING is true.
   *
   * \param success Optional reference to bool that will be set to true, if an element was successfully dequeued - false otherwise.
   * \return Element that was dequeued. NULL if no element could be dequeued, in case of pointers
   */
  inline T Dequeue(bool& success)
  {
    return tImplementation::Dequeue(success);
  }
  inline T Dequeue()
  {
    bool success = false;
    return tImplementation::Dequeue(success);
  }

  /*!
   * Add element to the end of the queue.
   * May only be called by a single writer threads concurrently, if CONCURRENT_ENQUEUEING is true.
   *
   * \param element Element to enqueue
   */
  inline void Enqueue(T && element)
  {
    tImplementation::Enqueue(std::forward<T>(element));
  }

  /*!
   * If queue is boundable, the maximum queue length can be set with this method.
   * (Should not be called by multiple threads concurrently in order to avoid strange side effects)
   *
   * Due to concurrency, the queue can temporarily become a little larger than the set.
   * If this length is exceeded, elements enqueued first are discarded.
   * Due to concurrency, however, the queue may temporarily contain more elements.
   * It is guaranteed that elements are discarded only if the queue length exceeds the specified 'guiding value'.
   *
   * \param max_length New 'guiding value' for maximum queue length (max. 500000 for concurrent queues)
   */
  template <bool ENABLE = BOUNDED>
  inline void SetMaxLength(typename std::enable_if<ENABLE, int>::type max_length)
  {
    tImplementation::SetMaxLength(max_length);
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  static_assert(sizeof(T) <= sizeof(void*), "Due to the use of universal queue node objects, sizeof(T) must not be larger than sizeof(void*).");
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
