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
template <typename T, tQueueConcurrency CONCURRENCY, bool BOUNDED>
class tQueueImplementation;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Unique pointer queues
/*!
 * Implementation for all queues dealing with unique pointers.
 */
template <typename T, typename D, tQueueConcurrency CONCURRENCY, bool BOUNDED, bool QUEUEABLE_TYPE, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation : public tQueueImplementation<T, CONCURRENCY, BOUNDED>
{
  // this combination of parameters is not supported yet - put pointers in ordinary queue

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef std::unique_ptr<T, D> tPointer;
  typedef tQueueImplementation<T, CONCURRENCY, BOUNDED> tBase;

  inline tPointer Dequeue()
  {
    bool success = false;
    return tPointer(tBase::Dequeue(success));
  }

  inline void Enqueue(tPointer && element)
  {
    tBase::Enqueue(element.release());
  }
};


///////////////////////////////////////////////////////////////////////////////
// Single threaded non-bounded queue implementations
///////////////////////////////////////////////////////////////////////////////


/*!
 * Single threaded implementation for tQueueableSingleThreaded
 */
template <typename T, typename D, bool QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, false, QUEUEABLE_TYPE, true> : public tQueueableSingleThreaded
{
//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef tQueueableSingleThreaded tElement;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tUniquePtrQueueImplementation() :
    last(this)
  {
    this->next_single_threaded_queueable = this;
  }

  inline tPointer Dequeue()
  {
    tElement* result = this->next_single_threaded_queueable;
    if (result == this)
    {
      return tPointer();
    }
    tElement* nextnext = result->next_single_threaded_queueable;
    if (!nextnext)    // now empty
    {
      last = this;
      nextnext = this;
    }
    this->next_single_threaded_queueable = nextnext;
    result->next_single_threaded_queueable = NULL;
    return tPointer(static_cast<T*>(result));
  }

  inline void Enqueue(tPointer && element)
  {
    last->next_single_threaded_queueable = element.get();
    last = element.release();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - never null */
  tElement* last;
};


/*!
 * Single threaded implementation for tQueueable
 */
template <typename T, typename D>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, false, true, false> : public tQueueable
{
//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef tQueueable tElement;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tUniquePtrQueueImplementation() :
    last(this)
  {
    this->next_queueable = this;
  }

  inline tPointer Dequeue()
  {
    tElement* result = this->next_queueable;
    if (result == this)
    {
      return tPointer();
    }
    tElement* nextnext = result->next_queueable;
    if (!nextnext)    // now empty
    {
      last = this;
      nextnext = this;
    }
    this->next_queueable = nextnext;
    result->next_queueable = NULL;
    return tPointer(static_cast<T*>(result));
  }

  inline void Enqueue(tPointer && element)
  {
    last->next_queueable = element.get();
    last = element.release();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - never null */
  tElement* last;
};

/*!
 * Bounded single-threaded queue implementation
 * Wraps unbounded single-threaded queue
 */
template <typename T, typename D, bool QUEUEABLE_TYPE, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tSingleThreadedBoundedUniquePtrQueueImplementation :
  public tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, false, QUEUEABLE_TYPE, SINGLE_THREADED_QUEUEABLE_TYPE>
{
//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, false, QUEUEABLE_TYPE, SINGLE_THREADED_QUEUEABLE_TYPE> tBase;
  typedef std::unique_ptr<T, D> tPointer;

  tSingleThreadedBoundedUniquePtrQueueImplementation() :
    element_count(0),
    max_length(std::numeric_limits<int>::max())
  {}

  inline tPointer Dequeue()
  {
    tPointer ptr = tBase::Dequeue();
    element_count -= (ptr) ? 1 : 0;
    return ptr;
  }

  inline void Enqueue(tPointer && element)
  {
    tBase::Enqueue(std::forward<tPointer>(element));
    element_count++;
    if (element_count > max_length)
    {
      Dequeue();
    }
  }

  void SetMaxLength(int max_length)
  {
    if (max_length <= 0)
    {
      RRLIB_LOG_PRINT(ERROR, "Invalid queue length: ", max_length, ". Ignoring.");
      return;
    }
    this->max_length = max_length;
    while (element_count > max_length)
    {
      Dequeue();
    }
  }

private:

  /*! Current number of elements in queue */
  int element_count;

  /*! Current maximum queue length */
  int max_length;
};


template <typename T, typename D>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, true, true, true> :
  public tSingleThreadedBoundedUniquePtrQueueImplementation<T, D, true, true>
{
};

template <typename T, typename D>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, true, false, true> :
  public tSingleThreadedBoundedUniquePtrQueueImplementation<T, D, false, true>
{
};

template <typename T, typename D>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, true, true, false> :
  public tSingleThreadedBoundedUniquePtrQueueImplementation<T, D, true, false>
{
};

///////////////////////////////////////////////////////////////////////////////
// Concurrent non-bounded queue implementations
///////////////////////////////////////////////////////////////////////////////


/*!
 * Base class for '_FAST' queue implementations: Single-threaded enqeueuing
 */
template <typename T, typename D, bool CONCURRENT>
class tFastUniquePtrQueueEnqueueImplementation : public tQueueable
{
//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  tFastUniquePtrQueueEnqueueImplementation() : last(this) {}

  ~tFastUniquePtrQueueEnqueueImplementation()
  {
    // delete last element
    if (last != this)
    {
      std::unique_ptr<T, D> last_dequeued(static_cast<T*>(last)); // will go out of scope an delete last element
    }
  }

  inline void Enqueue(std::unique_ptr<T, D> && element)
  {
    // swap last pointer
    tQueueable* prev = last;
    last = element.get();

    // set "next" of previous element
    assert(prev != last);
    prev->next_queueable = element.release();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - never null */
  tQueueable* last;

};

/*!
 * Base class for fast queue implementations: Concurrent enqeueuing
 */
template <typename T, typename D>
class tFastUniquePtrQueueEnqueueImplementation<T, D, true> : public tQueueable
{
//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  tFastUniquePtrQueueEnqueueImplementation() : last(this) {}

  ~tFastUniquePtrQueueEnqueueImplementation()
  {
    // delete last element
    if (last.load() != this)
    {
      std::unique_ptr<T, D> last_dequeued(static_cast<T*>(last.load())); // will go out of scope an delete last element
    }
  }

  inline void Enqueue(std::unique_ptr<T, D> && element)
  {
    // swap last pointer
    tQueueable* prev = last.exchange(element.get());

    // set "next" of previous element
    assert(prev != element.get());
    prev->next_queueable = element.release();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to last element in queue - never null */
  std::atomic<tQueueable*> last;

};

/*!
 * Base class for fast multi-reader implementations: Single-threaded dequeueing
 */
template <typename T, typename D, bool CONCURRENT_ENQUEUE, bool CONCURRENT_DEQUEUE>
class tFastUniquePtrQueueDequeueImplementation : public tFastUniquePtrQueueEnqueueImplementation<T, D, CONCURRENT_ENQUEUE>
{
//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 1 };

  tFastUniquePtrQueueDequeueImplementation()
  {
    this->next_queueable = &(tQueueable::terminator);
  }

  inline tPointer Dequeue()
  {
    tQueueable* result = this->next_queueable;
    tQueueable* nextnext = result->next_queueable;
    if (nextnext == &(tQueueable::terminator) || nextnext == NULL)
    {
      return tPointer();
    }
    this->next_queueable = nextnext;
    result->next_queueable = NULL;
    return tPointer(static_cast<T*>(result));
  }
};

/*!
 * Base class for fast multi-reader implementations: Concurrent Dequeueing
 */
template <typename T, typename D, bool CONCURRENT_ENQUEUE>
class tFastUniquePtrQueueDequeueImplementation<T, D, CONCURRENT_ENQUEUE, true> : public tFastUniquePtrQueueEnqueueImplementation<T, D, CONCURRENT_ENQUEUE>
{
  typedef rrlib::util::tTaggedPointer<tQueueable, false, 16> tFirstPointer;
  typedef typename tFirstPointer::tStorage tFirstPointerInt;

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 1 };

  tFastUniquePtrQueueDequeueImplementation() : first(tFirstPointer(this, 0))
  {
    this->next_queueable = NULL;
  }

  inline tPointer Dequeue()
  {
    tFirstPointer result = first.load();
    while (true)
    {
      tQueueable* nextnext = result->next_queueable;
      if (nextnext == this || nextnext == NULL)
      {
        return tPointer();
      }
      if (first.compare_exchange_strong(result, tFirstPointer(nextnext, (result.GetStamp() + 1) & tFirstPointer::cSTAMP_MASK)))
      {
        result->next_queueable = NULL;
        if (result.GetPointer() == this)
        {
          return Dequeue();
        }
        return tPointer(static_cast<T*>(result.GetPointer()));
      }
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*!
   * Atomic Pointer to first element in queue
   * Pointer tag counts number of dequeued elements => avoids ABA problem while dequeueing
   */
  std::atomic<tFirstPointerInt> first;

};

/*!
 * Implementation for tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST, false, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, false, false>
{
};

/*!
 * Implementation for tQueueConcurrency::MULTIPLE_WRITERS_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::MULTIPLE_WRITERS_FAST, false, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, true, false>
{
};

/*!
 * Implementation for tQueueConcurrency::MULTIPLE_READERS_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::MULTIPLE_READERS_FAST, false, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, false, true>
{
};

/*!
 * Implementation for tQueueConcurrency::FULL_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::FULL_FAST, false, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, true, true>
{
};

/*!
 * Implementation for tQueueConcurrency::MULTIPLE_WRITERS
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::MULTIPLE_WRITERS, false, true, SINGLE_THREADED_QUEUEABLE_TYPE> : public tQueueable
{

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tUniquePtrQueueImplementation() :
    last(this),
    read_last(this),
    next_first(NULL)
  {
    this->next_queueable = NULL;
  }

  inline tPointer Dequeue()
  {
    tQueueable* next = this->next_queueable;
    if (next == NULL)    // does readLast need updating?
    {
      next = next_first.exchange(NULL);
      if (next == NULL)
      {
        return tPointer();  // queue empty
      }
      read_last = last.exchange(this);
    }

    if (next == read_last)
    {
      this->next_queueable = NULL;
      return tPointer(static_cast<T*>(next));
    }
    tQueueable* nextnext = next->next_queueable;
    if (nextnext == NULL)    // can occur with delayed/preempted enqueue operations (next is set later and is not volatile)
    {
      return tPointer();  // queue is not empty, but elements are not fully available yet
    }
    this->next_queueable = nextnext;
    next->next_queueable = NULL;
    return tPointer(static_cast<T*>(next));
  }

  inline void Enqueue(tPointer && element)
  {
    // swap last pointer
    tQueueable* prev = last.exchange(element.get());

    if (prev == this)
    {
      assert(next_first.load() == NULL);
      next_first.store(element.release());
    }
    else
    {
      // set "next" of previous element
      prev->next_queueable = element.release();
    }
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*!
   * Pointer to last element in queue - never null
   * This variable us used for "communication" among writers
   */
  std::atomic<tQueueable*> last;

  /*!
   * Temporary last object (for dequeueing) - this way all elements can be dequeued.
   * points to this, if no element is available that has not been read already.
   */
  tQueueable* read_last;

  /*!
   * Next element after readLast
   *
   * This variable is used for "communication" between writer and reader.
   */
  std::atomic<tQueueable*> next_first;

};


///////////////////////////////////////////////////////////////////////////////
// Concurrent Bounded queue implementations
///////////////////////////////////////////////////////////////////////////////


/*!
 * Dequeue implementation for concurrent bounded queues (non-'_FAST')
 */
template <typename T, typename D, bool FAST>
class tUniquePtrBoundedDequeueImplementation : public tQueueable
{

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef rrlib::util::tTaggedPointer<tQueueable, true, 19> tTaggedPointer;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tUniquePtrBoundedDequeueImplementation() :
    first(tTaggedPointer(this, 0)),
    this_enqueued(true)
  {
    this->next_queueable = NULL;
  }

  template <typename TThis>
  inline tPointer Dequeue(TThis* thizz)
  {
    tTaggedPointer result = first.load();
    while (true)
    {
      tQueueable* nextnext = result->next_queueable;
      if (!nextnext)
      {
        // last element in queue... enqueue this?
        if (result.GetPointer() != this && this_enqueued.test_and_set() == false)
        {
          thizz->EnqueueRaw(this);
          // so... now we might be able to dequeue the other element
          nextnext = result->next_queueable;
        }
        if (!nextnext)
        {
          return tPointer();
        }
      }
      tTaggedPointer new_first(nextnext, (result.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (result.GetPointer() == this && nextnext)
      {
        if (first.compare_exchange_strong(result, new_first))
        {
          result->next_queueable = NULL;
          this_enqueued.clear();
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
      tQueueable* nextnext = first_element->next_queueable;
      if (!nextnext)
      {
        return;
      }
      tTaggedPointer new_first(nextnext, (first_element.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (first.compare_exchange_strong(first_element, new_first))
      {
        first_element->next_queueable = NULL;
        if (first_element.GetPointer() != this)
        {
          // discard element
          tPointer ptr(static_cast<T*>(first_element.GetPointer()));
        }
        else
        {
          this_enqueued.clear();
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

  /*!
   * Atomic Pointer to first element in queue.
   * Pointer tag counts number of dequeued elements.
   */
  std::atomic<tTaggedPointerRaw> first;

  /*!
   * True, if this is currently enqueued
   */
  std::atomic_flag this_enqueued;

};

/*!
 * Dequeue implementation for concurrent bounded queues ('_FAST')
 */
template <typename T, typename D>
class tUniquePtrBoundedDequeueImplementation<T, D, true> : public tQueueable
{

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef rrlib::util::tTaggedPointer<tQueueable, true, 19> tTaggedPointer;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 1 };

  tUniquePtrBoundedDequeueImplementation() :
    first(tTaggedPointer(this, 0))
  {
    this->next_queueable = NULL;
  }

  template <typename TThis>
  inline tPointer Dequeue(TThis* thizz)
  {
    tTaggedPointer result = first.load();
    while (true)
    {
      tQueueable* nextnext = result->next_queueable;
      if (!nextnext)
      {
        return tPointer();
      }
      tTaggedPointer new_first(nextnext, (result.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (first.compare_exchange_strong(result, new_first))
      {
        result->next_queueable = NULL;
        if (result.GetPointer() != this)
        {
          return tPointer(static_cast<T*>(result.GetPointer()));
        }
        result = new_first;
      }
    }
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
      tQueueable* nextnext = first_element->next_queueable;
      if (!nextnext)
      {
        return;
      }
      tTaggedPointer new_first(nextnext, (first_element.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);
      if (first.compare_exchange_strong(first_element, new_first))
      {
        first_element->next_queueable = NULL;
        if (first_element.GetPointer() != this)
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
class tUniquePtrBoundedEnqueueImplementation : public tUniquePtrBoundedDequeueImplementation<T, D, FAST>
{

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
public:

  typedef tUniquePtrBoundedDequeueImplementation<T, D, FAST> tBase;
  typedef typename tBase::tTaggedPointerRaw tTaggedPointerRaw;
  typedef typename tBase::tTaggedPointer tTaggedPointer;

  tUniquePtrBoundedEnqueueImplementation() : max_length(500000), last(tTaggedPointer(this, 0)), threads_enqeueuing(false) {}

  inline std::unique_ptr<T, D> Dequeue()
  {
    return tBase::Dequeue(this);
  }

  inline void EnqueueRaw(tQueueable* element)
  {
    threads_enqeueuing++;

    // swap last pointer
    bool this_ptr = element == static_cast<tQueueable*>(this);
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
class tUniquePtrBoundedEnqueueImplementation<T, D, false, true> : public tUniquePtrBoundedDequeueImplementation<T, D, true>
{

//----------------------------------------------------------------------
// Protected fields and methods
//----------------------------------------------------------------------
public:

  typedef tUniquePtrBoundedDequeueImplementation<T, D, true> tBase;
  typedef typename tBase::tTaggedPointer tTaggedPointer;

  tUniquePtrBoundedEnqueueImplementation() : max_length(500000), last(this, 0) {}

  inline std::unique_ptr<T, D> Dequeue()
  {
    return tBase::Dequeue(this);
  }

  inline void EnqueueRaw(tQueueable* element)
  {
    // swap last pointer
    tTaggedPointer prev = last;
    last = tTaggedPointer(element, (prev.GetStamp() + 1) & tTaggedPointer::cSTAMP_MASK);

    // set "next" of previous element
    assert(prev.GetPointer() != last.GetPointer && element != this);
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


template <typename T, typename D, tQueueConcurrency CONCURRENCY, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, CONCURRENCY, true, true, SINGLE_THREADED_QUEUEABLE_TYPE> : public tUniquePtrBoundedEnqueueImplementation < T, D,
  CONCURRENCY == tQueueConcurrency::MULTIPLE_WRITERS || CONCURRENCY == tQueueConcurrency::MULTIPLE_WRITERS_FAST || CONCURRENCY == tQueueConcurrency::FULL_FAST,
  CONCURRENCY == tQueueConcurrency::MULTIPLE_WRITERS_FAST || CONCURRENCY == tQueueConcurrency::FULL_FAST || CONCURRENCY == tQueueConcurrency::MULTIPLE_READERS_FAST  || CONCURRENCY == tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST >
{
public:

  inline void Enqueue(std::unique_ptr<T, D> && element)
  {
    EnqueueRaw(element.get());
    element.release();
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
      int max_len = this->max_length;
      TryDequeueingElementsOverBounds(this->GetLastStamp(), max_len, old_length - max_len);
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
