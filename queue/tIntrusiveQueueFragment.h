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
/*!\file    rrlib/concurrent_containers/queue/tIntrusiveQueueFragment.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tIntrusiveQueueFragment
 *
 * \b tIntrusiveQueueFragment
 *
 * Intrusive linked implementation of queue fragment.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tIntrusiveQueueFragment_h__
#define __rrlib__concurrent_containers__queue__tIntrusiveQueueFragment_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/util/tNoncopyable.h"
#include <memory>

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/tQueueable.h"

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
//! Intrusive linked queue fragment
/*!
 * Intrusive linked implementation of queue fragment.
 */
template <typename T, bool QUEUEABLE, bool SINGLE_THREADED_QUEUEABLE_TYPE>
class tIntrusiveQueueFragment
{
};

class tIntrusiveQueueFragmentQueueable : private rrlib::util::tNoncopyable
{
//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tIntrusiveQueueFragmentQueueable() : next_queueable(NULL), fifo_order(true), trim_to_size(-1), to_delete(NULL) {}

  /*! move constructor */
  tIntrusiveQueueFragmentQueueable(tIntrusiveQueueFragmentQueueable && other) :
    next_queueable(NULL), fifo_order(true), trim_to_size(-1), to_delete(NULL)
  {
    std::swap(next_queueable, other.next_queueable);
    std::swap(fifo_order, other.fifo_order);
    std::swap(trim_to_size, other.trim_to_size);
    std::swap(to_delete, other.to_delete);
  }

  /*! move assignment */
  tIntrusiveQueueFragmentQueueable& operator=(tIntrusiveQueueFragmentQueueable && other)
  {
    std::swap(next_queueable, other.next_queueable);
    std::swap(fifo_order, other.fifo_order);
    std::swap(trim_to_size, other.trim_to_size);
    std::swap(to_delete, other.to_delete);
    return *this;
  }

  ~tIntrusiveQueueFragmentQueueable()
  {
    assert((!to_delete) && (!next_queueable));
  }

  template <typename T, typename D>
  void DeleteObsoleteElements()
  {
    tQueueableMost* current = to_delete;
    while (current)
    {
      T* del = static_cast<T*>(current);
      current = current->next_queueable;
      del->next_queueable = NULL;
      std::unique_ptr<T, D> ptr(del);
    }
    trim_to_size = -1; // so that we pop all elements - even those beyond desired size
    to_delete = NULL;
  }

  bool Empty()
  {
    return next_queueable == NULL || trim_to_size == 0;
  }

  bool Fifo()
  {
    return fifo_order;
  }

  void InitLIFO(tQueueableMost* last, int max_length) // called from fragment-based queue
  {
    assert(max_length != 0);
    next_queueable = last;
    this->fifo_order = false;
    this->trim_to_size = max_length;
  }

  void InitFIFO(tQueueableMost* first) // only called from single-threaded queue
  {
    next_queueable = first;
    this->fifo_order = true;
    this->trim_to_size = -1;
  }

  tQueueableMost* PopAny()
  {
    if (Empty())
    {
      return NULL;
    }
    tQueueableMost* result = next_queueable;
    next_queueable = result->next_queueable;
    result->next_queueable = NULL;
    trim_to_size--; // we should not need an 'if > 0' here, since actually no risk of an underflow
    return result;
  }

  tQueueableMost* PopBack()
  {
    if (fifo_order)
    {
      Turn();
    }
    return PopAny();
  }

  tQueueableMost* PopFront()
  {
    if (!fifo_order)
    {
      Turn();
    }
    return PopAny();
  }

  bool PerformsSizeTrimming()
  {
    return trim_to_size >= 0;
  }

  void Turn();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  friend class tIntrusiveQueueFragmentQueueableSingleThreaded;

  /*! Next queueable */
  tQueueableMost* next_queueable;

  /*! Is queue fragment in fifo order? */
  bool fifo_order;

  /*!
   * Possibly there are more elements in this queue fragment than the maximum
   * length of the queue the fragment originates from.
   * If >= 0, only these many will be available from the fragment API.
   */
  int trim_to_size;

  /*! Chain of queueables to delete because they exceeded size */
  tQueueableMost* to_delete;
};

class tIntrusiveQueueFragmentQueueableSingleThreaded : private rrlib::util::tNoncopyable
{
//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  tIntrusiveQueueFragmentQueueableSingleThreaded() : next_queueable_single_threaded(NULL), fifo_order_single_threaded(true) {}

  /*! move constructor */
  tIntrusiveQueueFragmentQueueableSingleThreaded(tIntrusiveQueueFragmentQueueableSingleThreaded && other) :
    next_queueable_single_threaded(NULL), fifo_order_single_threaded(true)
  {
    std::swap(next_queueable_single_threaded, other.next_queueable_single_threaded);
    std::swap(fifo_order_single_threaded, other.fifo_order_single_threaded);
  }

  /*! move assignment */
  tIntrusiveQueueFragmentQueueableSingleThreaded& operator=(tIntrusiveQueueFragmentQueueableSingleThreaded && other)
  {
    std::swap(next_queueable_single_threaded, other.next_queueable_single_threaded);
    std::swap(fifo_order_single_threaded, other.fifo_order_single_threaded);
    return *this;
  }

  template <typename T, typename D>
  void DeleteObsoleteElements() {}

  bool Empty()
  {
    return next_queueable_single_threaded == NULL;
  }

  bool Fifo()
  {
    return fifo_order_single_threaded;
  }

  void InitSingleThreaded(tQueueableSingleThreaded* first, bool fifo_order)
  {
    next_queueable_single_threaded = first;
    this->fifo_order_single_threaded = fifo_order;
  }

  tQueueableSingleThreaded* PopAny()
  {
    if (!next_queueable_single_threaded)
    {
      return NULL;
    }
    tQueueableSingleThreaded* result = next_queueable_single_threaded;
    next_queueable_single_threaded = result->next_single_threaded_queueable;
    result->next_single_threaded_queueable = NULL;
    return result;
  }

  tQueueableSingleThreaded* PopBack()
  {
    if (fifo_order_single_threaded)
    {
      Turn();
    }
    return PopAny();
  }

  tQueueableSingleThreaded* PopFront()
  {
    if (!fifo_order_single_threaded)
    {
      Turn();
    }
    return PopAny();
  }

  void Turn();

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  /*! Next queueable */
  tQueueableSingleThreaded* next_queueable_single_threaded;

  /*! Is queue fragment in fifo order? */
  bool fifo_order_single_threaded;
};

template <typename T>
class tIntrusiveQueueFragment<T, true, false> : public tIntrusiveQueueFragmentQueueable
{
public:

  tIntrusiveQueueFragment() {}
  tIntrusiveQueueFragment(tIntrusiveQueueFragment && other) : tIntrusiveQueueFragmentQueueable(std::forward<tIntrusiveQueueFragmentQueueable>(other)) {}
  tIntrusiveQueueFragment& operator=(tIntrusiveQueueFragment && other)
  {
    tIntrusiveQueueFragmentQueueable::operator=(std::forward<tIntrusiveQueueFragmentQueueable>(other));
    return *this;
  }

  T* PopAny()
  {
    return static_cast<T*>(tIntrusiveQueueFragmentQueueable::PopAny());
  }

  T* PopBack()
  {
    return static_cast<T*>(tIntrusiveQueueFragmentQueueable::PopBack());
  }

  T* PopFront()
  {
    return static_cast<T*>(tIntrusiveQueueFragmentQueueable::PopFront());
  }
};

template <typename T>
class tIntrusiveQueueFragment<T, false, true> : public tIntrusiveQueueFragmentQueueableSingleThreaded
{
public:

  tIntrusiveQueueFragment() {}
  tIntrusiveQueueFragment(tIntrusiveQueueFragment && other) : tIntrusiveQueueFragmentQueueableSingleThreaded(std::forward<tIntrusiveQueueFragmentQueueableSingleThreaded>(other)) {}
  tIntrusiveQueueFragment& operator=(tIntrusiveQueueFragment && other)
  {
    tIntrusiveQueueFragmentQueueableSingleThreaded::operator=(std::forward<tIntrusiveQueueFragmentQueueableSingleThreaded>(other));
    return *this;
  }

  T* PopAny()
  {
    return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopAny());
  }

  T* PopBack()
  {
    return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopBack());
  }

  T* PopFront()
  {
    return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopFront());
  }
};


template <typename T>
class tIntrusiveQueueFragment<T, true, true> : public tIntrusiveQueueFragmentQueueable, public tIntrusiveQueueFragmentQueueableSingleThreaded
{
public:

  tIntrusiveQueueFragment() {}
  tIntrusiveQueueFragment(tIntrusiveQueueFragment && other) :
    tIntrusiveQueueFragmentQueueable(std::forward<tIntrusiveQueueFragmentQueueable>(other)),
    tIntrusiveQueueFragmentQueueableSingleThreaded(std::forward<tIntrusiveQueueFragmentQueueableSingleThreaded>(other))
  {}

  tIntrusiveQueueFragment& operator=(tIntrusiveQueueFragment && other)
  {
    tIntrusiveQueueFragmentQueueable::operator=(std::forward<tIntrusiveQueueFragmentQueueable>(other));
    tIntrusiveQueueFragmentQueueableSingleThreaded::operator=(std::forward<tIntrusiveQueueFragmentQueueableSingleThreaded>(other));
    return *this;
  }

  template <typename TP, typename D>
  void DeleteObsoleteElements()
  {
    tIntrusiveQueueFragmentQueueable::DeleteObsoleteElements<TP, D>();
  }

  bool Empty()
  {
    return tIntrusiveQueueFragmentQueueableSingleThreaded::Empty() && tIntrusiveQueueFragmentQueueable::Empty();
  }

  T* PopAny()
  {
    return tIntrusiveQueueFragmentQueueableSingleThreaded::Empty() ?
           static_cast<T*>(tIntrusiveQueueFragmentQueueable::PopAny()) :
           static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopAny());
  }

  T* PopBack()
  {
    if ((!tIntrusiveQueueFragmentQueueableSingleThreaded::Empty()) && (!tIntrusiveQueueFragmentQueueableSingleThreaded::Fifo()))
    {
      return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopAny());
    }
    else if ((!tIntrusiveQueueFragmentQueueable::Empty()) && (!tIntrusiveQueueFragmentQueueable::Fifo()))
    {
      return static_cast<T*>(tIntrusiveQueueFragmentQueueable::PopAny());
    }
    else if (!Empty())
    {
      Turn();
      assert(!tIntrusiveQueueFragmentQueueableSingleThreaded::Fifo());
      return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopAny());
    }
    return NULL;
  }

  T* PopFront()
  {
    if ((!tIntrusiveQueueFragmentQueueableSingleThreaded::Empty()) && tIntrusiveQueueFragmentQueueableSingleThreaded::Fifo())
    {
      return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopAny());
    }
    else if ((!tIntrusiveQueueFragmentQueueable::Empty()) && tIntrusiveQueueFragmentQueueable::Fifo())
    {
      return static_cast<T*>(tIntrusiveQueueFragmentQueueable::PopAny());
    }
    else if (!Empty())
    {
      Turn();
      assert(tIntrusiveQueueFragmentQueueableSingleThreaded::Fifo());
      return static_cast<T*>(tIntrusiveQueueFragmentQueueableSingleThreaded::PopAny());
    }
    return NULL;
  }

  void Turn()
  {
    if (!tIntrusiveQueueFragmentQueueableSingleThreaded::Empty())
    {
      tIntrusiveQueueFragmentQueueableSingleThreaded::Turn();
    }
    else
    {
      assert((!tIntrusiveQueueFragmentQueueable::Fifo() || (!PerformsSizeTrimming())) && "This only works under these conditions");
      T* first = static_cast<T*>(PopAny());
      T* current = first;
      T* next = static_cast<T*>(PopAny());
      while (next)
      {
        T* prev = current;
        current = next;
        next = static_cast<T*>(PopAny());
        current->next_single_threaded_queueable = prev;
      }
      tIntrusiveQueueFragmentQueueableSingleThreaded::InitSingleThreaded(current, !tIntrusiveQueueFragmentQueueable::Fifo());
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
