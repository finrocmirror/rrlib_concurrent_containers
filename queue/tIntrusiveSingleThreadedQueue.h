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
/*!\file    rrlib/concurrent_containers/queue/tIntrusiveSingleThreadedQueue.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tIntrusiveSingleThreadedQueue
 *
 * \b tIntrusiveSingleThreadedQueue
 *
 * Intrusive single-threaded linked queue implementations.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tIntrusiveSingleThreadedQueue_h__
#define __rrlib__concurrent_containers__queue__tIntrusiveSingleThreadedQueue_h__

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

/*!
 * Single threaded implementation for tQueueableSingleThreaded
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tBasicIntrusiveSingleThreadedQueue : public tQueueableSingleThreaded
{
//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef tQueueableSingleThreaded tElement;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tBasicIntrusiveSingleThreadedQueue() :
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

  inline tQueueFragment<tPointer> DequeueAll()
  {
    queue::tQueueFragmentImplementation<tPointer> result;
    if (this->next_single_threaded_queueable != this)
    {
      result.InitSingleThreaded(this->next_single_threaded_queueable, true);
    }
    this->next_single_threaded_queueable = this;
    last = this;
    return std::move(result);
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
class tBasicIntrusiveSingleThreadedQueue<T, D, false> : boost::noncopyable
{
public:

  typedef tQueueableMost tElement;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tBasicIntrusiveSingleThreadedQueue() : next(NULL), last(NULL) {}

  inline tPointer Dequeue()
  {
    tElement* result = this->next;
    if (!result)
    {
      return tPointer();
    }
    tElement* next = result->next_queueable;
    if (!next)    // now empty
    {
      last = NULL;
    }
    this->next = next;
    result->next_queueable = NULL;
    return tPointer(static_cast<T*>(result));
  }

  inline tQueueFragment<tPointer> DequeueAll()
  {
    queue::tQueueFragmentImplementation<tPointer> result;
    result.InitFIFO(this->next);
    next = NULL;
    last = NULL;
    return std::move(result);
  }

  inline void Enqueue(tPointer && element)
  {
    if (last) // if-condition is cheaper than setting an atomic
    {
      last->next_queueable = element.get();
    }
    else
    {
      next = element.get();
    }
    last = element.release();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Pointer to next element in queue - possibly null */
  tElement* next;

  /*! Pointer to last element in queue - possibly null */
  tElement* last;
};

/*!
 * Bounded single-threaded queue implementation
 * Wraps unbounded single-threaded queue
 */
template <typename T, typename D, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tBoundedIntrusiveSingleThreadedQueue : public tBasicIntrusiveSingleThreadedQueue<T, D, SINGLE_THREADED_QUEUEABLE_TYPE>
{
public:

  typedef tBasicIntrusiveSingleThreadedQueue<T, D, SINGLE_THREADED_QUEUEABLE_TYPE> tBase;
  typedef std::unique_ptr<T, D> tPointer;

  tBoundedIntrusiveSingleThreadedQueue() :
    element_count(0),
    max_length(std::numeric_limits<int>::max())
  {}

  inline tPointer Dequeue()
  {
    tPointer ptr = tBase::Dequeue();
    element_count -= (ptr) ? 1 : 0;
    return ptr;
  }

  inline tQueueFragment<tPointer> DequeueAll()
  {
    element_count = 0;
    return tBase::DequeueAll();
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

  int GetMaxLength() const
  {
    return max_length;
  }

  void SetMaxLength(int max_length)
  {
    if (max_length < 0)
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

  int Size()
  {
    return element_count;
  }

private:

  /*! Current number of elements in queue */
  int element_count;

  /*! Current maximum queue length */
  int max_length;
};

//! Intrusive single-threaded linked queue implementations.
/*!
 * Intrusive single-threaded linked queue implementations.
 */
template <typename T, typename D, bool BOUNDED, bool QUEUEABLE_TYPE, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tIntrusiveSingleThreadedQueue : public tBasicIntrusiveSingleThreadedQueue<T, D, SINGLE_THREADED_QUEUEABLE_TYPE>
{
  static_assert(QUEUEABLE_TYPE || SINGLE_THREADED_QUEUEABLE_TYPE, "Intrusive queue only available for types derived from tQueueable<...>");
};

template <typename T, typename D, bool QUEUEABLE_TYPE, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tIntrusiveSingleThreadedQueue<T, D, true, QUEUEABLE_TYPE, SINGLE_THREADED_QUEUEABLE_TYPE> :
  public tBoundedIntrusiveSingleThreadedQueue<T, D, SINGLE_THREADED_QUEUEABLE_TYPE>
{
  static_assert(QUEUEABLE_TYPE || SINGLE_THREADED_QUEUEABLE_TYPE, "Intrusive queue only available for types derived from tQueueable<...>");
};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
