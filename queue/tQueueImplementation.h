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
/*!\file    rrlib/concurrent_containers/queue/tQueueImplementation.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-25
 *
 * \brief   Contains tQueueImplementation
 *
 * \b tQueueImplementation
 *
 * Implementations for different types of queues.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tQueueImplementation_h__
#define __rrlib__concurrent_containers__queue__tQueueImplementation_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueable.h"
#include "rrlib/concurrent_containers/tQueueFragment.h"
#include "rrlib/concurrent_containers/queue/tUniquePtrQueueImplementation.h"

//----------------------------------------------------------------------
// Namespace declaration
//----------------------------------------------------------------------
namespace rrlib
{
namespace concurrent_containers
{
namespace queue
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Queue Implementation
/*!
 * Implementations for different types of queues.
 */
template <typename T, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tQueueImplementation
{
  // this combination of parameters is not supported yet
};

template <tDequeueMode DEQUEUE_MODE>
struct tUniquePtrQueueElementDeleter
{
  template <typename QUEUE>
  static void DeleteElements(QUEUE& queue)
  {
    // Since we have unique pointers enqueued, we need to delete all enqueued elements
    bool success = true;
    while (success)
    {
      queue.Dequeue(success);
    }
    //TODO: DeleteLast() ?
  }
};

template <>
struct tUniquePtrQueueElementDeleter<tDequeueMode::ALL>
{
  template <typename QUEUE>
  static void DeleteElements(QUEUE& queue)
  {
    queue.DequeueAll();
  }
};

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tQueueImplementation<std::unique_ptr<T, D>, CONCURRENCY, DEQUEUE_MODE, BOUNDED> :
  public tUniquePtrQueueImplementation<T, D, CONCURRENCY, DEQUEUE_MODE, BOUNDED, std::is_base_of<tQueueableMost, T>::value>
{
  typedef tUniquePtrQueueImplementation<T, D, CONCURRENCY, DEQUEUE_MODE, BOUNDED, std::is_base_of<tQueueableMost, T>::value> tBase;

  static_assert(sizeof(std::unique_ptr<T, D>) == sizeof(void*), "Only unique pointers with Deleter of size 0 may be used in queue. Otherwise, this would be too much info to store in an atomic.");

public:

  ~tQueueImplementation()
  {
    tUniquePtrQueueElementDeleter<DEQUEUE_MODE>::DeleteElements(*this);
  }

  inline std::unique_ptr<T, D> Dequeue(bool& success)
  {
    std::unique_ptr<T, D> ptr = tBase::Dequeue();
    success = ptr.get();
    return std::move(ptr);
  }

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
