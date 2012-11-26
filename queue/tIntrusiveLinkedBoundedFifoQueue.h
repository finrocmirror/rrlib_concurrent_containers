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
/*!\file    rrlib/concurrent_containers/queue/tIntrusiveLinkedBoundedFifoQueue.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tIntrusiveLinkedBoundedFifoQueue
 *
 * \b tIntrusiveLinkedBoundedFifoQueue
 *
 * Bounded concurrent intrusive linked queue implementations.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tIntrusiveLinkedBoundedFifoQueue_h__
#define __rrlib__concurrent_containers__queue__tIntrusiveLinkedBoundedFifoQueue_h__

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
//! Bounded concurrent intrusive linked queue implementations.
/*!
 * Bounded concurrent intrusive linked queue implementations.
 */
template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedBoundedFifoQueue;

/*!
 * Dequeue implementation for concurrent bounded queues (non-'FAST')
 */
template <typename T, typename D, bool FAST>
class tIntrusiveLinkedBoundedDequeueImplementation : public boost::noncopyable
{
public:

  typedef rrlib::util::tTaggedPointer<tQueueableMost, true, 19> tTaggedPointer;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tIntrusiveLinkedBoundedDequeueImplementation() :
    fill_element(),
    fill_element_enqueued(true),
    first(tTaggedPointer(&fill_element, 0))
  {
  }

  template <typename TThis>
  inline tPointer Dequeue(TThis* thizz)
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
          thizz->EnqueueRaw(&fill_element);
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

  tQueueableMost& InitialElement()
  {
    return fill_element;
  }

  /*!
   * Attempt to dequeue elements that exceed max length
   * If another threads interferes - abort attempt
   * Dequeue max 10 elements
   */
  void TryDequeueingElementsOverBounds(int last_stamp, int max_length, int max_elements_to_dequeue)
  {
    tTaggedPointer first_element = first.load();
    int dequeued = 0;
    while (dequeued < max_elements_to_dequeue)
    {
      int diff = last_stamp - first_element.GetStamp();
      if (diff < 0)
      {
        diff += (1 << 19);
      }
      if (diff < max_length)
      {
        return;
      }

      // dequeue one element
      tQueueableMost* nextnext = first_element->next_queueable;
      if (!nextnext)
      {
        return;
      }
      tTaggedPointer new_first(nextnext, (first_element.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (first.compare_exchange_strong(first_element, new_first))
      {
        first_element->next_queueable = NULL;
        if (first_element.GetPointer() != &fill_element)
        {
          // discard element
          tPointer ptr(static_cast<T*>(first_element.GetPointer()));
        }
        else
        {
          fill_element_enqueued.clear();
        }
        first_element = new_first;
        dequeued++;
      }
      else
      {
        // another thread interfered
        return;
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
 * Dequeue implementation for concurrent bounded queues ('FAST')
 */
template <typename T, typename D>
class tIntrusiveLinkedBoundedDequeueImplementation<T, D, true> : public boost::noncopyable
{
public:

  typedef rrlib::util::tTaggedPointer<tQueueableMost, true, 19> tTaggedPointer;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 1 };

  tIntrusiveLinkedBoundedDequeueImplementation() :
    initial_element(),
    first(tTaggedPointer(&initial_element, 0))
  {
  }

  inline tPointer Dequeue(void* thizz)
  {
    tTaggedPointer result = first.load();
    while (true)
    {
      tQueueableMost* nextnext = result->next_queueable;
      if (!nextnext)
      {
        return tPointer();
      }
      tTaggedPointer new_first(nextnext, (result.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (first.compare_exchange_strong(result, new_first))
      {
        result->next_queueable = NULL;
        if (result.GetPointer() != &initial_element)
        {
          return tPointer(static_cast<T*>(result.GetPointer()));
        }
        result = new_first;
      }
    }
  }

  tQueueableMost& InitialElement()
  {
    return initial_element;
  }

  /*!
   * Attempt to dequeue elements that exceed max length
   * If another threads interferes - abort attempt
   * Dequeue max 10 elements
   */
  void TryDequeueingElementsOverBounds(int last_stamp, int max_length, int max_elements_to_dequeue)
  {
    tTaggedPointer first_element = first.load();
    int dequeued = 0;
    while (dequeued < max_elements_to_dequeue)
    {
      int diff = last_stamp - first_element.GetStamp();
      if (diff < 0)
      {
        diff += (1 << 19);
      }
      if (diff <= max_length) // '<=' because we have '_FAST' queue that contains at least one element
      {
        return;
      }

      // dequeue one element
      tQueueableMost* nextnext = first_element->next_queueable;
      if (!nextnext)
      {
        return;
      }
      tTaggedPointer new_first(nextnext, (first_element.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (first.compare_exchange_strong(first_element, new_first))
      {
        first_element->next_queueable = NULL;
        if (first_element.GetPointer() != &initial_element)
        {
          // discard element
          tPointer ptr(static_cast<T*>(first_element.GetPointer()));
        }
        first_element = new_first;
        dequeued++;
      }
      else
      {
        // another thread interfered
        return;
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Initial element when queue is otherwise empty */
  __attribute__((aligned(8)))  // mysterious why it has an offset to this
  tQueueableMost initial_element;

  /*!
   * Atomic Pointer to first element in queue.
   * Pointer tag counts number of dequeued elements.
   */
  std::atomic<tTaggedPointerRaw> first;
};


/*!
 * Enqueue implementation for multiple-writer concurrent queues
 */
template <typename T, typename D, bool CONCURRENT, bool FAST>
class tIntrusiveLinkedBoundedEnqueueImplementation : public tIntrusiveLinkedBoundedDequeueImplementation<T, D, FAST>
{

public:

  typedef tIntrusiveLinkedBoundedDequeueImplementation<T, D, FAST> tBase;
  typedef typename tBase::tTaggedPointerRaw tTaggedPointerRaw;
  typedef typename tBase::tTaggedPointer tTaggedPointer;

  tIntrusiveLinkedBoundedEnqueueImplementation() : max_length(500000), last(tTaggedPointer(&this->InitialElement(), 0)), threads_enqeueuing(false) {}

  ~tIntrusiveLinkedBoundedEnqueueImplementation()
  {
    tTaggedPointer l = last.load();
    if (FAST && l.GetPointer() != &this->InitialElement())
    {
      std::unique_ptr<T, D> ptr(static_cast<T*>(l.GetPointer()));
    }
  }

  inline std::unique_ptr<T, D> Dequeue()
  {
    return tBase::Dequeue(this);
  }

  inline void EnqueueRaw(tQueueableMost* element)
  {
    threads_enqeueuing++;

    // swap last pointer
    bool this_ptr = element == static_cast<tQueueableMost*>(&this->InitialElement());
    tTaggedPointer prev = last.load();
    tTaggedPointer new_last(element, (prev.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
    while (!last.compare_exchange_strong(prev, new_last))
    {
      new_last.SetStamp((prev.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
    }

    // set "next" of previous element
    assert(prev.GetPointer() != element);
    prev->next_queueable = element;

    int threads_enqueuing_tmp = --threads_enqeueuing;

    // dequeue some elements?
    if (threads_enqueuing_tmp == 0 && (!this_ptr))
    {
      // all threads completed setting 'next' up to current stamp
      TryDequeueingElementsOverBounds(new_last.GetStamp(), max_length, 10);
    }
  }

  int GetLastStamp()
  {
    tTaggedPointer temp = last.load();
    return temp.GetStamp();
  }

  /*! Maximum queue length */
  std::atomic<int> max_length;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - tagged with counter of already enqueued elements */
  std::atomic<tTaggedPointerRaw> last;

  /*! Threads currently enqeueuing elements */
  std::atomic<int> threads_enqeueuing;
};

/*!
 * Enqueue implementation for single-writer concurrent queues
 */
template <typename T, typename D>
class tIntrusiveLinkedBoundedEnqueueImplementation<T, D, false, true> : public tIntrusiveLinkedBoundedDequeueImplementation<T, D, true>
{

public:

  typedef tIntrusiveLinkedBoundedDequeueImplementation<T, D, true> tBase;
  typedef typename tBase::tTaggedPointer tTaggedPointer;

  tIntrusiveLinkedBoundedEnqueueImplementation() : max_length(500000), last(&this->InitialElement(), 0) {}

  ~tIntrusiveLinkedBoundedEnqueueImplementation()
  {
    if (last.GetPointer() != &this->InitialElement())
    {
      std::unique_ptr<T, D> ptr(static_cast<T*>(last.GetPointer()));
    }
  }

  inline std::unique_ptr<T, D> Dequeue()
  {
    return tBase::Dequeue(this);
  }

  inline void EnqueueRaw(tQueueableMost* element)
  {
    // swap last pointer
    tTaggedPointer prev = last;
    last = tTaggedPointer(element, (prev.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);

    // set "next" of previous element
    assert(prev.GetPointer() != last.GetPointer() && element != &this->InitialElement());
    prev->next_queueable = element;

    // dequeue some elements?
    TryDequeueingElementsOverBounds(last.GetStamp(), max_length, 10);
  }

  int GetLastStamp()
  {
    return last.GetStamp();
  }

  /*! Maximum queue length */
  std::atomic<int> max_length;

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - tagged with counter of already enqueued elements */
  tTaggedPointer last;
};

template <typename T, typename D, tConcurrency CONCURRENCY, tDequeueMode DEQUEUE_MODE>
class tIntrusiveLinkedBoundedFifoQueue :
  public tIntrusiveLinkedBoundedEnqueueImplementation < T, D, CONCURRENCY == tConcurrency::MULTIPLE_WRITERS || CONCURRENCY == tConcurrency::FULL, DEQUEUE_MODE == tDequeueMode::FIFO_FAST >
{
public:

  inline void Enqueue(std::unique_ptr<T, D> && element)
  {
    EnqueueRaw(element.get());
    element.release();
  }

  int GetMaxLength() const
  {
    return this->max_length;
  }

  void SetMaxLength(int max_length)
  {
    if (max_length <= 0 || max_length > 500000)
    {
      RRLIB_LOG_PRINT(ERROR, "Invalid queue length: ", this->max_length.load(), ". Ignoring.");
      return;
    }
    int old_length = this->max_length.exchange(max_length);
    if (max_length < old_length)
    {
      TryDequeueingElementsOverBounds(this->GetLastStamp(), max_length, old_length - max_length);
    }
  }
};


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
