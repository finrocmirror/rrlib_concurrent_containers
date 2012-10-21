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
/*!\file    rrlib/concurrent_containers/queue/tUniquePtrQueueImplementation.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-09-26
 *
 * \brief   Contains tUniquePtrQueueImplementation
 *
 * \b tUniquePtrQueueImplementation
 *
 * Implementation for all queues dealing with unique pointers.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tUniquePtrQueueImplementation_h__
#define __rrlib__concurrent_containers__queue__tUniquePtrQueueImplementation_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/util/tTaggedPointer.h"

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/queue/tIntrusiveSingleThreadedQueue.h"
#include "rrlib/concurrent_containers/queue/tIntrusiveLinkedFifoQueue.h"
#include "rrlib/concurrent_containers/queue/tIntrusiveLinkedBoundedFifoQueue.h"
#include "rrlib/concurrent_containers/queue/tIntrusiveLinkedFragmentBasedQueue.h"

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
template <typename T, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tQueueImplementation;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Unique pointer queues
/*!
 * Implementation for all queues dealing with unique pointers.
 */
template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED, bool QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation : public tQueueImplementation<T, CONCURRENCY, DEQUEUE_MODE, BOUNDED>
{
  // this combination of parameters is not supported yet - TODO: put pointers in ordinary queue

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
  /*public:

    typedef std::unique_ptr<T, D> tPointer;
    typedef tQueueImplementation<T, CONCURRENCY, DEQUEUE_MODE, BOUNDED> tBase;

    inline tPointer Dequeue()
    {
      bool success = false;
      return tPointer(tBase::Dequeue(success));
    }

    inline void Enqueue(tPointer && element)
    {
      tBase::Enqueue(element.release());
    }*/
};

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tConcurrentIntrusiveQueue;

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tUniquePtrQueueImplementation<T, D, CONCURRENCY, DEQUEUE_MODE, BOUNDED, true> :
  public tConcurrentIntrusiveQueue<T, D, CONCURRENCY, DEQUEUE_MODE, BOUNDED>
{};

///////////////////////////////////////////////////////////////////////////////
// Single threaded queue implementations
///////////////////////////////////////////////////////////////////////////////

template <typename T, typename D, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tUniquePtrQueueImplementation<T, D, tConcurrency::NONE, DEQUEUE_MODE, BOUNDED, true> :
  public tIntrusiveSingleThreadedQueue<T, D, BOUNDED, true, std::is_base_of<tQueueableSingleThreaded, T>::value>
{
};

template <typename T, typename D, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tUniquePtrQueueImplementation<T, D, tConcurrency::NONE, DEQUEUE_MODE, BOUNDED, false> :
  public tIntrusiveSingleThreadedQueue<T, D, BOUNDED, false, std::is_base_of<tQueueableSingleThreaded, T>::value>
{
};

///////////////////////////////////////////////////////////////////////////////
// Concurrent non-bounded queue implementations
///////////////////////////////////////////////////////////////////////////////
template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tConcurrentIntrusiveFifoQueue;

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tConcurrentIntrusiveQueue :
  public tConcurrentIntrusiveFifoQueue<T, D, CONCURRENCY, DEQUEUE_MODE, BOUNDED>
{
};

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE, bool BOUNDED>
class tConcurrentIntrusiveFifoQueue : public tIntrusiveLinkedFifoQueue<T, D, CONCURRENCY, DEQUEUE_MODE>
{};

///////////////////////////////////////////////////////////////////////////////
// Concurrent bounded queue implementations
///////////////////////////////////////////////////////////////////////////////

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE>
class tConcurrentIntrusiveFifoQueue<T, D, CONCURRENCY, DEQUEUE_MODE, true> : public tIntrusiveLinkedBoundedFifoQueue<T, D, CONCURRENCY, DEQUEUE_MODE>
{};

///////////////////////////////////////////////////////////////////////////////
// Concurrent fragment-based queue
///////////////////////////////////////////////////////////////////////////////

template <typename T, typename D, tConcurrency CONCURRENCY, bool BOUNDED>
class tConcurrentIntrusiveQueue<T, D, CONCURRENCY, tDequeueMode::ALL, BOUNDED> :
  public tIntrusiveLinkedFragmentBasedQueue<T, D, CONCURRENCY, BOUNDED>
{
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}

#endif
