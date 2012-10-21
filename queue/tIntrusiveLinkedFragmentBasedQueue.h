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
/*!\file    rrlib/concurrent_containers/queue/tIntrusiveLinkedFragmentBasedQueue.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tIntrusiveLinkedFragmentBasedQueue
 *
 * \b tIntrusiveLinkedFragmentBasedQueue
 *
 * Concurrent intrusive linked queue implementations for tDequeueMode::ALL
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tIntrusiveLinkedFragmentBasedQueue_h__
#define __rrlib__concurrent_containers__queue__tIntrusiveLinkedFragmentBasedQueue_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <cstdint>

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
//! Concurrent intrusive linked queue implementations for tDequeueMode::ALL
/*!
 * Concurrent intrusive linked queue implementations for tDequeueMode::ALL
 * (default non-bounded implementation)
 */
template <typename T, typename D, tConcurrency CONCURRENCY, bool BOUNDED>
class tIntrusiveLinkedFragmentBasedQueue : boost::noncopyable
{

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tIntrusiveLinkedFragmentBasedQueue() : last(NULL) {}

  inline tQueueFragment<tPointer> DequeueAll()
  {
    queue::tQueueFragmentImplementation<tPointer> result;
    tQueueableMost* ex_last = last.exchange(NULL);
    result.InitLIFO(ex_last, -1);
    return std::move(result);
  }

  inline void Enqueue(tPointer && element)
  {
    tQueueableMost* current_last = last.load();
    assert(current_last != element.get());
    do
    {
      element->next_queueable = current_last;
    }
    while (!last.compare_exchange_strong(current_last, element.get()));

    element.release();
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Last element in queue */
  std::atomic<tQueueableMost*> last;
};

#if INTPTR_MAX == INT32_MAX

/*!
 * Bounded implementation:
 *
 * We need two stamps:
 * (1) One to count enqueue operations in order to avoid ABA problems (13 bit)
 * (2) One to track queue chunk length (19 bit)
 *
 * On 32 bit platforms we store both in this queue's stamped 'last' pointer.
 * On 64 bit platforms we store (1) in this queue's stamped 'last' pointer and (2) tQueueableFull's 'queueable_pointer' stamp.
 *
 * This is the 32 bit implementation
 */
template <typename T, typename D, tConcurrency CONCURRENCY>
class tIntrusiveLinkedFragmentBasedQueue<T, D, CONCURRENCY, true> : boost::noncopyable
{
  static_assert(std::is_base_of<tQueueableFull, T>::value, "T needs to be derived from tQueueable<FULL> or tQueueable<FULL_OPTIMIZED> for this kind of queue.");

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef rrlib::util::tTaggedPointer<tQueueableFull, true, 32> tTaggedPointer;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };
  enum { cCOUNTER_MASK = 0x1FFF };
  //enum { cCHUNKLEN_MASK = 0xFFFFE000 };
  //enum { cCHUNKLEN_MASK_INCREMENT = 0x2000 };

  tIntrusiveLinkedFragmentBasedQueue() : last(0), max_length(500000) {}

  inline tQueueFragment<tPointer> DequeueAll()
  {
    queue::tQueueFragmentImplementation<tPointer> result;
    tTaggedPointer ex_last = last.load();
    while (ex_last.GetPointer() && (!last.compare_exchange_strong(ex_last, tTaggedPointer(NULL, ex_last.GetStamp() & cCOUNTER_MASK)))); // keep counter in 'last'
    tQueueableFull* ex_last_ptr = ex_last.GetPointer();

    // remove link after first full chunk
    if (ex_last_ptr)
    {
      tQueueableFull* ex_last_ptr2 = static_cast<tQueueableFull*>(ex_last_ptr->queueable_pointer.load()->next_queueable.load());
      if (ex_last_ptr2)
      {
        ex_last_ptr2->queueable_pointer.load()->next_queueable = NULL;
      }
    }

    result.InitLIFO(ex_last_ptr, max_length);
    return std::move(result);
  }

  inline void Enqueue(tPointer && element)
  {
    uint max_len = max_length;
    tTaggedPointer current_last = last.load();
    assert(current_last.GetPointer() != element.get());
    while (true)
    {
      tQueueableFull* current_last_ptr = current_last.GetPointer();
      uint current_last_stamp = current_last.GetStamp();
      uint current_chunk_len = current_last_stamp >> 13;
      bool new_chunk = current_chunk_len >= max_len;
      tQueueableMost* chunk_to_delete = NULL;
      if (!new_chunk)
      {
        // append to this chunk
        element->next_queueable = current_last_ptr;
        element->queueable_pointer = current_last_ptr ? current_last_ptr->queueable_pointer.load() : element.get();
      }
      else
      {
        // start new chunk
        element->next_queueable = current_last_ptr;
        element->queueable_pointer = element.get();
        current_chunk_len = 0;
        chunk_to_delete = current_last_ptr->queueable_pointer.load()->next_queueable; // last element of chunk this thread is responsible of deleting - should compare_exchange_strong succeed
      }
      uint next_last_stamp = ((current_chunk_len + 1) << 13) | ((current_last_stamp + 1) & cCOUNTER_MASK); // increase counter and chunk length
      tTaggedPointer new_last(element.get(), next_last_stamp);
      if (last.compare_exchange_strong(current_last, new_last))
      {
        element.release();

        // possibly delete old chunk
        if (chunk_to_delete)
        {
          tQueueableMost* first = static_cast<tQueueableFull*>(chunk_to_delete)->queueable_pointer;
          while (chunk_to_delete != first)
          {
            tQueueableMost* temp = chunk_to_delete;
            chunk_to_delete = chunk_to_delete->next_queueable;
            temp->next_queueable = NULL;
            tPointer p(static_cast<T*>(temp));
          }
          chunk_to_delete->next_queueable = NULL;
          tPointer p(static_cast<T*>(chunk_to_delete));
        }

        return;
      }
    }
  }

  void SetMaxLength(int max_length)
  {
    if (max_length <= 0 || max_length > 500000)
    {
      RRLIB_LOG_PRINT(ERROR, "Invalid queue length: ", this->max_length.load(), ". Ignoring.");
      return;
    }
    this->max_length = max_length;
    // Can we safely shorten queue here? (I don't think so)
    /*int old_length = this->max_length.exchange(max_length);
    if (max_length < old_length)
    {
      ??
    }*/
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Last element (tag counts elements in current chunk) */
  std::atomic<tTaggedPointerRaw> last;

  /*! 'Maximum length' of queue (fragments) */
  std::atomic<int> max_length;
};

#elif INTPTR_MAX == INT64_MAX

/*!
 * 64 bit implementation
 */
template <typename T, typename D, tConcurrency CONCURRENCY>
class tIntrusiveLinkedFragmentBasedQueue<T, D, CONCURRENCY, true> : boost::noncopyable
{
  static_assert(std::is_base_of<tQueueableFull, T>::value, "T needs to be derived from tQueueable<FULL> or tQueueable<FULL_OPTIMIZED> for this kind of queue.");

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  typedef rrlib::util::tTaggedPointer<tQueueableFull, true, 16> tTaggedPointer;
  typedef rrlib::util::tTaggedPointer<tQueueableFull, true, 19> tTaggedPointer2;
  typedef typename tTaggedPointer::tStorage tTaggedPointerRaw;
  typedef std::unique_ptr<T, D> tPointer;
  enum { cMINIMUM_ELEMENTS_IN_QEUEUE = 0 };

  tIntrusiveLinkedFragmentBasedQueue() : last(0), max_length(500000) {}

  inline tQueueFragment<tPointer> DequeueAll()
  {
    queue::tQueueFragmentImplementation<tPointer> result;
    tTaggedPointer ex_last = last.load();
    while (ex_last.GetPointer() && (!last.compare_exchange_strong(ex_last, tTaggedPointer(NULL, ex_last.GetStamp())))); // keep counter in 'last'
    tQueueableFull* ex_last_ptr = ex_last.GetPointer();

    // remove link after first full chunk
    if (ex_last_ptr)
    {
      tTaggedPointer2 first_in_current_chunk(ex_last_ptr->queueable_tagged_pointer.load());
      tQueueableFull* ex_last_ptr2 = static_cast<tQueueableFull*>(first_in_current_chunk->next_queueable.load());
      if (ex_last_ptr2)
      {
        tTaggedPointer2 first_in_last_chunk(ex_last_ptr2->queueable_tagged_pointer.load());
        first_in_last_chunk->next_queueable = NULL;
      }
    }

    result.InitLIFO(ex_last_ptr, max_length);
    return std::move(result);
  }

  inline void Enqueue(tPointer && element)
  {
    uint max_len = max_length;
    tTaggedPointer current_last = last.load();
    assert(current_last.GetPointer() != element.get());
    while (true)
    {
      tQueueableFull* current_last_ptr = current_last.GetPointer();
      uint current_last_stamp = current_last.GetStamp();
      uint64_t queueable_tagged_ptr_raw = current_last_ptr ? current_last_ptr->queueable_tagged_pointer.load() : 0;
      tTaggedPointer2 queueable_tagged_ptr(queueable_tagged_ptr_raw);
      tQueueableFull* queueable_ptr = queueable_tagged_ptr.GetPointer();
      uint current_chunk_len = queueable_tagged_ptr.GetStamp();
      bool new_chunk = current_chunk_len >= max_len;
      tQueueableMost* chunk_to_delete = NULL;
      if (!new_chunk)
      {
        // append to this chunk
        element->next_queueable = current_last_ptr;
        element->queueable_tagged_pointer = tTaggedPointer2(queueable_ptr ? queueable_ptr : element.get(), current_chunk_len + 1);
      }
      else
      {
        // start new chunk
        element->next_queueable = current_last_ptr;
        element->queueable_tagged_pointer = tTaggedPointer2(element.get(), 1);
        chunk_to_delete = queueable_ptr->next_queueable; // last element of chunk this thread is responsible of deleting - should compare_exchange_strong succeed
      }
      tTaggedPointer new_last(element.get(), ((current_last_stamp + 1) & tTaggedPointer::cSTAMP_MASK));
      if (last.compare_exchange_strong(current_last, new_last))
      {
        element.release();

        // possibly delete old chunk
        if (chunk_to_delete)
        {
          tTaggedPointer2 first_tagged_ptr = static_cast<tQueueableFull*>(chunk_to_delete)->queueable_tagged_pointer.load();
          tQueueableMost* first = first_tagged_ptr.GetPointer();
          while (chunk_to_delete != first)
          {
            tQueueableMost* temp = chunk_to_delete;
            chunk_to_delete = chunk_to_delete->next_queueable;
            temp->next_queueable = NULL;
            tPointer p(static_cast<T*>(temp));
          }
          chunk_to_delete->next_queueable = NULL;
          tPointer p(static_cast<T*>(chunk_to_delete));
        }

        return;
      }
    }
  }

  void SetMaxLength(int max_length)
  {
    if (max_length <= 0 || max_length > 500000)
    {
      RRLIB_LOG_PRINT(ERROR, "Invalid queue length: ", this->max_length.load(), ". Ignoring.");
      return;
    }
    this->max_length = max_length;
    // Can we safely shorten queue here? (I don't think so)
    /*int old_length = this->max_length.exchange(max_length);
    if (max_length < old_length)
    {
      ??
    }*/
  }

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Last element (tag counts enqeue operations) */
  std::atomic<tTaggedPointerRaw> last;

  /*! 'Maximum length' of queue (fragments) */
  std::atomic<int> max_length;
};

#else
#error "Is platform neither 32 nor 64-bit?"
#endif

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
