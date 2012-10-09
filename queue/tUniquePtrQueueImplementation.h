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
template <typename T, tQueueConcurrency CONCURRENCY>
class tQueueImplementation;

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Unique pointer queues
/*!
 * Implementation for all queues dealing with unique pointers.
 */
template <typename T, typename D, tQueueConcurrency CONCURRENCY, bool QUEUEABLE_TYPE, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation : public tQueueImplementation<T, CONCURRENCY>
{
  // this combination of parameters is not supported yet - put pointers in ordinary queue

//----------------------------------------------------------------------
// Protected methods
//----------------------------------------------------------------------
protected:

  typedef std::unique_ptr<T, D> tPointer;
  typedef tQueueImplementation<T, CONCURRENCY> tBase;

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


/*!
 * Single threaded implementation for tQueueableSingleThreaded
 */
template <typename T, typename D, bool QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, QUEUEABLE_TYPE, true> : public tQueueableSingleThreaded
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
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::NONE, true, false> : public tQueueable
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
 * Base class for fast queue implementations: Single-threaded enqeueuing
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
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::SINGLE_READER_AND_WRITER_FAST, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, false, false>
{
};

/*!
 * Implementation for tQueueConcurrency::MULTIPLE_WRITERS_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::MULTIPLE_WRITERS_FAST, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, true, false>
{
};

/*!
 * Implementation for tQueueConcurrency::MULTIPLE_READERS_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::MULTIPLE_READERS_FAST, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, false, true>
{
};

/*!
 * Implementation for tQueueConcurrency::FULL_FAST
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::FULL_FAST, true, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tFastUniquePtrQueueDequeueImplementation<T, D, true, true>
{
};

/*!
 * Implementation for tQueueConcurrency::MULTIPLE_WRITERS
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tUniquePtrQueueImplementation<T, D, tQueueConcurrency::MULTIPLE_WRITERS, true, SINGLE_THREADED_QUEUEABLE_TYPE> : public tQueueable
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



//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}

#endif
