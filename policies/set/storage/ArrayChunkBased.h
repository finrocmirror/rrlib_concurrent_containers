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
/*!\file    rrlib/concurrent_containers/policies/set/storage/ArrayChunkBased.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-07
 *
 * \brief   Contains ArrayChunkBased
 *
 * \b tArrayChunkBased
 *
 * Set storage based on singly-linked array chunks.
 * Set entries are stored in array chunks:
 *  Whenever the capacity is insufficient, another array chunk is appended.
 * This policy is quite efficient with respect to memory footprint,
 * if set size does not exceed the initial size often.
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__policies__set__storage__ArrayChunkBased_h__
#define __rrlib__concurrent_containers__policies__set__storage__ArrayChunkBased_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include <atomic>
#include <array>
#include "rrlib/thread/tLock.h"

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
namespace set
{
namespace storage
{

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Set storage based on singly-linked array chunks.
/*!
 * Set storage based on singly-linked array chunks.
 * Set entries are stored in array chunks:
 *  Whenever the capacity is insufficient, another array chunk is appended.
 * This policy is quite efficient with respect to memory footprint,
 * if set size does not exceed the initial size often.
 *
 * \tparam INITIAL_CHUNK_SIZE Entries/slots in initial chunk
 * \tparam SECOND_CHUNK_SIZE Entries/slots in first appended chunk
 * \tparam FURTHER_CHUNKS_SIZE Entries/slots in any further appended chunks
 * \tparam CHUNK_SIZE_INCREASE_FACTOR Second appended chunk will have a size of SECOND_CHUNKS_SIZE * CHUNK_SIZE_INCREASE_FACTOR.
 *                                    The third chunk's size will be increased by this factor again.
 * \tparam SINGLE_THREADED Is tSet in a single-threaded context only?
 *                         Setting this, the set is actually no longer suitable for concurrency.
 *                         However, this option is provided so that tSet can conveniently
 *                         be used in templates that might sometimes be used a single-threaded context.
 */
template <size_t INITIAL_CHUNK_SIZE, size_t SECOND_CHUNK_SIZE, size_t FURTHER_CHUNKS_SIZE, bool SINGLE_THREADED = false>
struct ArrayChunkBased
{

  template <typename T, tAllowDuplicates ALLOW_DUPLICATES, typename TMutex, typename TNullElement>
  class tInstance : public TMutex
  {
    /*! The set storage is a linked list of array chunks */
    template <size_t SIZE, bool INITIAL = false>
    struct tArrayChunk;

    typedef tArrayChunk<INITIAL_CHUNK_SIZE, true> tFirstChunk;
    typedef tArrayChunk<SECOND_CHUNK_SIZE> tSecondChunk;
    typedef tArrayChunk<FURTHER_CHUNKS_SIZE> tFurtherChunk;
    typedef typename std::conditional<SINGLE_THREADED, tSecondChunk*, std::atomic<tSecondChunk*>>::type tSecondChunkPointer;
    typedef typename std::conditional<SINGLE_THREADED, tFurtherChunk*, std::atomic<tFurtherChunk*>>::type tFurtherChunkPointer;
    typedef typename std::conditional<SINGLE_THREADED, T, std::atomic<T>>::type tArrayElement;
    typedef typename std::conditional<SINGLE_THREADED, size_t, std::atomic<size_t>>::type tSize;

    template <size_t SIZE, bool INITIAL>
    struct tArrayChunk
    {
      typedef typename std::conditional<INITIAL, tSecondChunk, tFurtherChunk>::type tNextChunk;
      typedef typename std::conditional<INITIAL, tSecondChunkPointer, tFurtherChunkPointer>::type tNextChunkPointer;

      /*! Buffers in array chunk. NULL for buffers that are in use. */
      std::array<tArrayElement, SIZE> buffers;

      /*! Pointer to next chunk -> linked-list */
      tNextChunkPointer next_chunk;

      static_assert(sizeof(buffers) % sizeof(next_chunk) == 0, "Please choose a chunk size that does not waste memory");

      ~tArrayChunk()
      {
        tNextChunk* next = next_chunk;
        delete next;
      }
    };

    //----------------------------------------------------------------------
    // Public methods and typedefs
    //----------------------------------------------------------------------
  public:

    // Iterator types
    template <bool CONST>
    class tIteratorExternal;
    typedef tIteratorExternal<false> tIterator;
    typedef tIteratorExternal<true> tConstIterator;

    tInstance() : size(0) {}

    void Add(const T& element)
    {
      rrlib::thread::tLock lock(*this);

      // Check for duplicates
      tArrayElement* first_free = NULL;
      tIteratorInternal<false> it(*this);
      for (; it != tIteratorInternal<false>(); ++it)
      {
        if (ALLOW_DUPLICATES == tAllowDuplicates::NO && (*it) == element)
        {
          return;
        }
        else if ((!first_free) && (*it) == TNullElement::cNULL_ELEMENT)
        {
          first_free = it.current_array_entry;
          if (ALLOW_DUPLICATES != tAllowDuplicates::NO)
          {
            break;
          }
        }
      }

      // insert
      if (first_free)
      {
        (*first_free) = element;
        //size += 0; // TODO:
      }
      else
      {
        if ((void*)it.past_last_array_entry != (void*)it.next_chunk)
        {
          *it.past_last_array_entry = element;
        }
        else
        {
          if (it.initial_chunk)
          {
            tSecondChunk* new_chunk = new tSecondChunk();
            *it.second_chunk = new_chunk;
            new_chunk->buffers[0] = element;
          }
          else
          {
            tFurtherChunk* new_chunk = new tFurtherChunk();
            *it.next_chunk = new_chunk;
            new_chunk->buffers[0] = element;
          }
        }
        size++; // important: do this last
      }
    }

    tIterator Begin()
    {
      return tIterator(*this);
    }
    tConstIterator Begin() const
    {
      return tConstIterator(*this);
    }

    tIterator End()
    {
      return tIterator();
    }
    tConstIterator End() const
    {
      return tConstIterator();
    }

    tIterator Remove(tIterator position)
    {
      rrlib::thread::tLock lock(*this);
      *(position.current_array_entry) = TNullElement::cNULL_ELEMENT;
      ++position;
      if (position == End())
      {
        // last element? Check, by how much we can decrease size  // TODO: optimize
        size_t free_slots_at_back = 0;
        for (auto it = tIteratorInternal<false>(*this); it != tIteratorInternal<false>(); ++it)
        {
          if ((*it) == TNullElement::cNULL_ELEMENT)
          {
            free_slots_at_back++;
          }
          else
          {
            free_slots_at_back = 0;
          }
        }
        if (free_slots_at_back)
        {
          size -= free_slots_at_back;
        }
      }
      return position;
    }

    void Remove(const T& element)
    {
      rrlib::thread::tLock lock(*this);
      tIteratorInternal<false> it(*this);
      size_t free_slots_at_back = 0;
      for (; it != tIteratorInternal<false>(); ++it)
      {
        if ((*it) == element)
        {
          (*it) = TNullElement::cNULL_ELEMENT;
          free_slots_at_back++;
        }
        else if ((*it) == TNullElement::cNULL_ELEMENT)
        {
          free_slots_at_back++;
        }
        else
        {
          free_slots_at_back = 0;
        }
      }
      if (free_slots_at_back)
      {
        size -= free_slots_at_back;
      }
    }

    /*! Iterator base implementation */
    template <bool CONST>
    class tIteratorImplementation : public std::iterator<std::forward_iterator_tag, T, size_t>
    {
      typedef std::iterator<std::forward_iterator_tag, T, size_t> tBase;

    public:

      // Operators needed for C++ Input Iterator

      inline typename tBase::reference operator*()
      {
        assert(remaining);
        return current_element;
      }
      inline typename tBase::pointer operator->()
      {
        return &(operator*());
      }

      inline tIteratorImplementation& operator++()
      {
        remaining--;
        current_array_entry++;
        if (current_array_entry < past_last_array_entry)
        {
          current_element = *current_array_entry;
        }
        else if (remaining)
        {
          if (SECOND_CHUNK_SIZE != FURTHER_CHUNKS_SIZE && initial_chunk)
          {
            *this = tIteratorImplementation(**second_chunk, remaining);
          }
          else
          {
            *this = tIteratorImplementation(**next_chunk, remaining);
          }
        }
        else
        {
          current_array_entry = NULL;
        }
        return *this;
      }
      inline tIteratorImplementation operator ++ (int)
      {
        tIteratorImplementation temp(*this);
        operator++();
        return temp;
      }

      inline const bool operator == (const tIteratorImplementation &other) const
      {
        return current_array_entry == other.current_array_entry;
      }
      inline const bool operator != (const tIteratorImplementation &other) const
      {
        return !(*this == other);
      }

    protected:

      template <size_t SIZE, bool INITIAL>
      tIteratorImplementation(tArrayChunk<SIZE, INITIAL>& chunk, size_t set_size) :
        current_array_entry(set_size ? (&chunk.buffers[0]) : NULL),
        past_last_array_entry((&chunk.buffers[std::min(SIZE, set_size)])),
        remaining(set_size),
        next_chunk_raw(&chunk.next_chunk),
        initial_chunk(INITIAL),
        current_element(remaining ? static_cast<T>(*current_array_entry) : TNullElement::cNULL_ELEMENT)
      {
      }

      template <size_t SIZE, bool INITIAL>
      tIteratorImplementation(const tArrayChunk<SIZE, INITIAL>& chunk, size_t set_size) :
        current_array_entry(set_size ? (&chunk.buffers[0]) : NULL),
        past_last_array_entry((&chunk.buffers[std::min(SIZE, set_size)])),
        remaining(set_size),
        next_chunk_raw(&chunk.next_chunk),
        initial_chunk(INITIAL),
        current_element(remaining ? static_cast<T>(*current_array_entry) : TNullElement::cNULL_ELEMENT)
      {
      }

      tIteratorImplementation() :
        current_array_entry(NULL),
        past_last_array_entry(NULL),
        remaining(0),
        next_chunk(),
        initial_chunk(),
        current_element(TNullElement::cNULL_ELEMENT)
      {
      }

    private:

      friend class tInstance;

      /*! Pointer to current element */
      typename std::conditional<CONST, const tArrayElement*, tArrayElement*>::type current_array_entry;

      /*! Last element in array chunk */
      typename std::conditional<CONST, const tArrayElement*, tArrayElement*>::type past_last_array_entry;

    protected:

      /*! Remaining elements in set (including current element => 0 means that iterator has passed the end) */
      size_t remaining;

    private:

      /*! Pointer to next chunk */
      union
      {
        const void* next_chunk_raw;
        tFurtherChunkPointer* next_chunk;
        tSecondChunkPointer* second_chunk;
      };

      /*! Initial chunk? */
      bool initial_chunk;

      /*! Current element */
      T current_element;

    };

    /*! Internal iterator (for insside this class file only) - includes null entries */
    template <bool CONST>
    class tIteratorInternal : public tIteratorImplementation<CONST>
    {
      friend class tInstance;
      template <size_t X = INITIAL_CHUNK_SIZE>
      tIteratorInternal(typename std::enable_if<X, typename std::conditional<CONST, const tInstance, tInstance>::type>::type& instance) : tIteratorImplementation<CONST>(instance.first_chunk, instance.size) {}
      template <size_t X = INITIAL_CHUNK_SIZE>
      tIteratorInternal(typename std::enable_if < !X, typename std::conditional<CONST, const tInstance, tInstance>::type >::type& instance) : tIteratorImplementation<CONST>(*instance.first_chunk.next_chunk, instance.size) {}
      tIteratorInternal() : tIteratorImplementation<CONST>() {}
    };

    /*! External iterator (for use by users of set) - excludes null entries */
    template <bool CONST>
    class tIteratorExternal : public tIteratorInternal<CONST>
    {
    public:
      tIteratorExternal(typename std::conditional<CONST, const tInstance, tInstance>::type& instance) : tIteratorInternal<CONST>(instance)
      {
        if (**this == TNullElement::cNULL_ELEMENT)
        {
          operator++();
        }
      }

      tIteratorExternal() : tIteratorInternal<CONST>() {}

      inline tIteratorExternal& operator++()
      {
        do
        {
          tIteratorInternal<CONST>::operator++();
        }
        while (this->remaining && **this == TNullElement::cNULL_ELEMENT);
        return *this;
      }
      inline tIteratorExternal operator ++ (int)
      {
        tIteratorExternal temp(*this);
        operator++();
        return temp;
      }
    };

    //----------------------------------------------------------------------
    // Private fields and methods
    //----------------------------------------------------------------------
  private:

    /*! First Chunk */
    tFirstChunk first_chunk;

    /*! Number of slots used */
    tSize size;
  };

};

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}
}


#endif
