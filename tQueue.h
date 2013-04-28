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
 * Concurrent non-blocking Queue.
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
#include "rrlib/concurrent_containers/tConcurrency.h"
#include "rrlib/concurrent_containers/tDequeueMode.h"
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
//! Concurrent non-blocking Queue.
/*!
 * This is a concurrent non-blocking linked queue.
 * It should be suitable for real-time code, as it does not need to allocate memory.
 *
 * Depending on the template parameters, it allows concurrent enqueueing
 * and dequeueing operations.
 * There is no size limit - unless BOUNDED is true.
 *
 * Using this queue is most efficient, when using std::unique_ptr<U> as type T, with U
 * derived from tQueueable<...>.
 * TODO: Otherwise, objects are placed in queue nodes that need to be managed separately.
 * Due to the use of universal queue node objects, sizeof(T) must not be larger than sizeof(void*).
 *
 * \tparam T Enqueued elements. Ideally, std::unique_ptr<U> with with U derived from tQueueable<...>
 * \tparam CONCURRENCY Concurrency that queue should support
 *                     (#writers = #threads that can enqueue elements concurrently)
 *                     (#readers = #threads that can dequeue elements concurrently)
 * \tparam DEQUEUE_MODE Determines how elements can be dequeued from the queue.
 * \tparam BOUNDED If true, a 'guiding value' for maximum queue length can be specified.
 *                 It can be changed at runtime (up to 500K elements).
 *                 If this length is exceeded, elements enqueued first are discarded.
 *                 Due to concurrency and depending on the implementation, however, the queue may contain more elements
 *                 (up to twice the 'guiding value').
 *                 It is guaranteed that elements are discarded only if the queue length exceeds the specified 'guiding value'.
 */
template <typename T, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED = false>
class tQueue
{
  typedef queue::tQueueImplementation<T, CONCURRENCY, DEQUEUE_MODE, BOUNDED> tImplementation;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  /*! Type of enqueued elements */
  typedef T tElement;

  /*!
   * Minimum number of elements in queue.
   * In 'tDequeueMode::FIFO_FAST' queue implementations this is typically one - meaning that the last
   * element cannot be dequeued (it can, as soon as another element is enqueued).
   * Otherwise it's zero.
   */
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = tImplementation::cMINIMUM_ELEMENTS_IN_QEUEUE };

  /*!
   * (Available if DEQUEUE_MODE is FIFO or FIFO_FAST)
   * Remove first element from queue and return it.
   * May only be called by multiple threads concurrently, if selected CONCURRENCY allows multiple readers.
   *
   * \param success Optional reference to bool that will be set to true, if an element was successfully dequeued - false otherwise.
   * \param unused Unused parameter for std::enable_if (simply ignore)
   * \return Element that was dequeued. NULL if no element could be dequeued, in case of pointers
   */
  template < bool ENABLE = (DEQUEUE_MODE != tDequeueMode::ALL) >
  inline T Dequeue(bool& success, typename std::enable_if<ENABLE, void>::type* unused = NULL)
  {
    return implementation.Dequeue(success);
  }
  template < bool ENABLE = (DEQUEUE_MODE != tDequeueMode::ALL) >
  inline T Dequeue(typename std::enable_if<ENABLE, void>::type* unused = NULL)
  {
    bool success = false;
    return implementation.Dequeue(success);
  }

  /*!
   * (Available if DEQUEUE_MODE is ALL)
   * Remove all available elements in queue and return in a 'queue fragment'.
   * May only be called by multiple threads concurrently, if selected CONCURRENCY allows multiple readers.
   *
   * \param success Optional reference to bool that will be set to true, if an element was successfully dequeued - false otherwise.
   * \param unused Unused parameter for std::enable_if (simply ignore)
   * \return Fragment containing all elements that were in queue. If queue is bounded, contains no more elements than was set via SetMaxLength.
   */
  template <bool ENABLE = (DEQUEUE_MODE == tDequeueMode::ALL)>
  inline tQueueFragment<T> DequeueAll(typename std::enable_if<ENABLE, void>::type* unused = NULL)
  {
    return implementation.DequeueAll();
  }

  /*!
   * Add element to the end of the queue.
   * May only be called by multiple threads concurrently, if selected CONCURRENCY allows multiple writers.
   *
   * \param element Element to enqueue
   */
  inline void Enqueue(T && element)
  {
    implementation.Enqueue(std::forward<T>(element));
  }
  inline void Enqueue(T& element)
  {
    implementation.Enqueue(std::move(element));
  }

  /*!
   * If queue is boundable, the maximum queue length can be retrieved with this method.
   */
  template <bool ENABLE = BOUNDED>
  inline typename std::enable_if<ENABLE, int>::type GetMaxLength() const
  {
    return implementation.GetMaxLength();
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
    implementation.SetMaxLength(max_length);
  }

  /*!
   * \return Number of elements in queue (currently only available for bounded, single-threaded queues)
   */
  template < bool ENABLE = BOUNDED && (CONCURRENCY == tConcurrency::NONE) >
  typename std::enable_if<ENABLE, int>::type Size()
  {
    return implementation.Size();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  static_assert(sizeof(T) <= sizeof(void*), "Due to the use of universal queue node objects, sizeof(T) must not be larger than sizeof(void*).");

  /*! Queue implementation */
  tImplementation implementation;
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
