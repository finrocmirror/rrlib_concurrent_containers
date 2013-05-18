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
/*!\file    rrlib/concurrent_containers/queue/tIntrusiveLinkedFifoQueue.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tIntrusiveLinkedFifoQueue
 *
 * \b tIntrusiveLinkedFifoQueue
 *
 * Concurrent intrusive non-bounded linked queue implementations
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tIntrusiveLinkedFifoQueue_h__
#define __rrlib__concurrent_containers__queue__tIntrusiveLinkedFifoQueue_h__

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
namespace queue
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Concurrent intrusive non-bounded linked queue implementations
/*!
 * Concurrent intrusive non-bounded linked queue implementations
 */
template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedFifoQueue
{
};

/*!
 * Base class for concurrent non-bounded enqueueing: Single-threaded implementation
 */
template <typename T, typename D, bool CONCURRENT>
class tFastIntrusiveEnqueueImplementation : private rrlib::util::tNoncopyable
{
public:

  tFastIntrusiveEnqueueImplementation(tQueueableMost* initial_last) : last(initial_last) {}

  /*!
   * For internal purposes when deleting 'FAST' queue: Deletes last element if it is not the specified element
   */
  inline void DeleteLastElement(tQueueableMost& ignore)
  {
    // delete last element
    if (last != &ignore)
    {
      std::unique_ptr<T, D> last_dequeued(static_cast<T*>(last)); // will go out of scope an delete last element
    }
  }

  inline void Enqueue(std::unique_ptr<T, D> && element)
  {
    EnqueueRaw(element.release());
  }

  inline void EnqueueRaw(tQueueableMost* element)
  {
    // swap last pointer
    tQueueableMost* prev = last;
    last = element;

    // set "next" of previous element
    assert(prev != last);
    prev->next_queueable = element;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - never null */
  tQueueableMost* last;

};

/*!
 * Base class for concurrent non-bounded enqueueing: Concurrent implementation
 */
template <typename T, typename D>
class tFastIntrusiveEnqueueImplementation<T, D, true> : private rrlib::util::tNoncopyable
{
public:

  tFastIntrusiveEnqueueImplementation(tQueueableMost* initial_last) : last(initial_last) {}

  /*!
   * For internal purposes when deleting 'FAST' queue: Deletes last element if it is not the specified element
   */
  inline void DeleteLastElement(tQueueableMost& ignore)
  {
    // delete last element
    if (last.load() != &ignore)
    {
      std::unique_ptr<T, D> last_dequeued(static_cast<T*>(last.load())); // will go out of scope an delete last element
    }
  }

  inline void Enqueue(std::unique_ptr<T, D> && element)
  {
    EnqueueRaw(element.release());
  }

  inline void EnqueueRaw(tQueueableMost* element)
  {
    // swap last pointer
    tQueueableMost* prev = last.exchange(element);

    // set "next" of previous element
    assert(prev != element);
    prev->next_queueable = element;
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - never null */
  std::atomic<tQueueableMost*> last;

};

/*!
 * Base class for concurrent non-bounded dequeueing: concurrent, non-'FAST' dequeueing
 */
template <typename T, typename D, bool CONCURRENT_ENQUEUE, bool CONCURRENT_DEQUEUE, bool FAST>
class tFastIntrusiveDequeueImplementation : public tFastIntrusiveEnqueueImplementation<T, D, true>
{
public:

  typedef rrlib::util::tTaggedPointer<tQueueableMost, true, 19> tTaggedPointer;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tFastIntrusiveDequeueImplementation() :
    tFastIntrusiveEnqueueImplementation<T, D, true>(&fill_element),
    fill_element(),
    fill_element_enqueued(true),
    first(tTaggedPointer(&fill_element, 0))
  {
  }

  inline tPointer Dequeue()
  {
    tTaggedPointer result = first.load();
    while (true)
    {
      tQueueableMost* nextnext = result->next_queueable;
      if (!nextnext)
      {
        // last element in queue... enqueue this?
        if (result.GetPointer() != &fill_element && fill_element_enqueued.test_and_set() == false)
        {
          this->EnqueueRaw(&fill_element);
          // so... now we might be able to dequeue the other element
          nextnext = result->next_queueable;
        }
        if (!nextnext)
        {
          return tPointer();
        }
      }
      tTaggedPointer new_first(nextnext, (result.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (result.GetPointer() == &fill_element && nextnext)
      {
        if (first.compare_exchange_strong(result, new_first))
        {
          result->next_queueable = NULL;
          fill_element_enqueued.clear();
          result = new_first;
        }
      }
      else
      {
        if (first.compare_exchange_strong(result, new_first))
        {
          result->next_queueable = NULL;
          return tPointer(static_cast<T*>(result.GetPointer()));
        }
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Dummy fill element to be able to dequeue all elements */
  tQueueableMost fill_element;

  /*! True, if fill element is currently enqueued */
  std::atomic_flag fill_element_enqueued;

  /*!
   * Atomic Pointer to first element in queue.
   * Pointer tag counts number of dequeued elements.
   */
  std::atomic<tTaggedPointerRaw> first;
};

/*!
 * Base class for non-bounded dequeueing: single-threaded, non-'FAST' dequeueing
 */
template <typename T, typename D, bool CONCURRENT_ENQUEUE>
class tFastIntrusiveDequeueImplementation<T, D, CONCURRENT_ENQUEUE, false, false> : public tFastIntrusiveEnqueueImplementation<T, D, true>
{
public:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tFastIntrusiveDequeueImplementation() :
    tFastIntrusiveEnqueueImplementation<T, D, true>(&fill_element),
    fill_element(),
    fill_element_enqueued(true),
    first(&fill_element)
  {
  }

  inline tPointer Dequeue()
  {
    tQueueableMost* result = first;
    while (true)
    {
      tQueueableMost* next = result->next_queueable;
      if (!next)
      {
        // last element in queue... enqueue this?
        if (result != &fill_element && fill_element_enqueued == false)
        {
          this->EnqueueRaw(&fill_element);
          fill_element_enqueued = true;
          // so... now we might be able to dequeue the other element
          next = result->next_queueable;
        }
        if (!next)
        {
          return tPointer();
        }
      }
      assert(next);
      first = next;
      result->next_queueable = NULL;
      if (result == &fill_element)
      {
        fill_element_enqueued = false;
        result = next;
      }
      else
      {
        return tPointer(static_cast<T*>(result));
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Dummy fill element to be able to dequeue all elements */
  tQueueableMost fill_element;

  /*! True, if fill element is currently enqueued */
  bool fill_element_enqueued;

  /*!
   * Atomic Pointer to first element in queue.
   * Pointer tag counts number of dequeued elements.
   */
  tQueueableMost* first;
};

/*!
 * Base class for concurrent non-bounded dequeueing: Single-threaded, fast dequeueing
 */
template <typename T, typename D, bool CONCURRENT_ENQUEUE>
class tFastIntrusiveDequeueImplementation<T, D, CONCURRENT_ENQUEUE, false, true> : public tFastIntrusiveEnqueueImplementation<T, D, CONCURRENT_ENQUEUE>
{
public:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 1 };

  tFastIntrusiveDequeueImplementation() :
    tFastIntrusiveEnqueueImplementation<T, D, CONCURRENT_ENQUEUE>(&initial_element),
    initial_element(),
    first(NULL)
  {
  }

  ~tFastIntrusiveDequeueImplementation()
  {
    this->DeleteLastElement(initial_element);
  }

  inline tPointer Dequeue()
  {
    tQueueableMost* result = first ? first : initial_element.next_queueable.load();
    tQueueableMost* nextnext = result ? result->next_queueable.load() : NULL;
    if (nextnext == NULL)
    {
      return tPointer();
    }
    first = nextnext;
    result->next_queueable = NULL;
    return tPointer(static_cast<T*>(result));
  }

private:

  /*! Initial element in queue */
  tQueueableMost initial_element;

  /*! First element in queue */
  tQueueableMost* first;
};

/*!
 * Base class for concurrent non-bounded dequeueing: Concurrent, fast dequeueing
 */
template <typename T, typename D, bool CONCURRENT_ENQUEUE>
class tFastIntrusiveDequeueImplementation<T, D, CONCURRENT_ENQUEUE, true, true> : public tFastIntrusiveEnqueueImplementation<T, D, CONCURRENT_ENQUEUE>
{
  typedef rrlib::util::tTaggedPointer<tQueueableMost, false, 16> tFirstPointer;
  typedef typename tFirstPointer::tStorage tFirstPointerInt;

public:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 1 };

  tFastIntrusiveDequeueImplementation() :
    tFastIntrusiveEnqueueImplementation<T, D, CONCURRENT_ENQUEUE>(&initial_element),
    initial_element(),
    first(tFirstPointer(NULL, 0))
  {
  }

  ~tFastIntrusiveDequeueImplementation()
  {
    this->DeleteLastElement(initial_element);
  }

  inline tPointer Dequeue()
  {
    tFirstPointer first_pointer = first.load();
    tQueueableMost* result = first_pointer ? first_pointer.GetPointer() : initial_element.next_queueable.load();
    while (true)
    {
      tQueueableMost* nextnext = result ? result->next_queueable.load() : NULL;
      if (nextnext == NULL)
      {
        return tPointer();
      }
      if (first.compare_exchange_strong(first_pointer, tFirstPointer(nextnext, (first_pointer.GetStamp() + 1) & tFirstPointer::cSTAMP_MASK)))
      {
        result->next_queueable = NULL;
        return tPointer(static_cast<T*>(result));
      }
      else
      {
        result = first_pointer.GetPointer();
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Initial element in queue */
  tQueueableMost initial_element;

  /*!
   * Atomic Pointer to first element in queue
   * Pointer tag counts number of dequeued elements => avoids ABA problem while dequeueing
   */
  std::atomic<tFirstPointerInt> first;

};


template <typename T, typename D, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedFifoQueue<T, D, tConcurrency::SINGLE_READER_AND_WRITER, DEQUEUE_MODE> :
  public tFastIntrusiveDequeueImplementation<T, D, false, false, DEQUEUE_MODE == tDequeueMode::FIFO_FAST>
{
};

template <typename T, typename D, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedFifoQueue<T, D, tConcurrency::MULTIPLE_WRITERS, DEQUEUE_MODE> :
  public tFastIntrusiveDequeueImplementation<T, D, true, false, DEQUEUE_MODE == tDequeueMode::FIFO_FAST>
{
};

template <typename T, typename D, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedFifoQueue<T, D, tConcurrency::MULTIPLE_READERS, DEQUEUE_MODE> :
  public tFastIntrusiveDequeueImplementation<T, D, false, true, DEQUEUE_MODE == tDequeueMode::FIFO_FAST>
{
};

template <typename T, typename D, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedFifoQueue<T, D, tConcurrency::FULL, DEQUEUE_MODE> :
  public tFastIntrusiveDequeueImplementation<T, D, true, true, DEQUEUE_MODE == tDequeueMode::FIFO_FAST>
{
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
