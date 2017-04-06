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
/*!\file    rrlib/concurrent_containers/tRegister.h
 *
 * \author  Max Reichardt
 *
 * \date    2017-02-04
 *
 * \brief   Contains tRegister
 *
 * \b tRegister
 *
 * A growing register - often global.
 * This is a data structure that is actually required quite often in the RRLib/Finroc context
 * (e.g. register of data types or create actions)
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tRegister_h__
#define __rrlib__concurrent_containers__tRegister_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/util/tNoncopyable.h"
#include <atomic>
#include <functional>
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

//----------------------------------------------------------------------
// Forward declarations / typedefs / enums
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Register
/*!
 * A growing register - often global.
 * This is a data structure that is actually required quite often in the RRLib/Finroc context
 * (e.g. register of data types or create actions)
 *
 * Regarding modifications, only new elements can be added.
 * Size query, element lookup via index, and iteration can be done concurrently to modifications.
 * Size query is very efficient.
 * Memory is organized in chunks so register can flexibly allocate further memory when required.
 *
 * \tparam TEntry Elements of register. Can be any type with a default constructor that could also be used in a std::vector
 * \tparam Tchunk_count Number of chunks. Tchunk_count * Tchunk_size is the maximum number of elements in the register.
 * \tparam Tchunk_size Size of chunks. Tchunk_count * Tchunk_size is the maximum number of elements in the register. A value that is a power of 2 will make register most efficient.
 * \tparam TMutex Mutex for synchronizing concurrent modifications (internally)
 */
template <typename TEntry, size_t Tchunk_count, size_t Tchunk_size, typename TMutex = rrlib::thread::tMutex>
class tRegister : public TMutex
{

  static_assert(Tchunk_count >= 1 && Tchunk_size >= 1, "Invalid Tchunk_count or Tchunk_size");

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
public:

  class tConstIterator;
  typedef TEntry tEntry;
  enum { cCHUNK_COUNT = Tchunk_count };
  enum { cCHUNK_SIZE = Tchunk_size };
  enum { cCAPACITY = Tchunk_count * Tchunk_size };

  /*!
   * \param external_size_variable External size variable (optional - can be placed in header for efficient size lookup)
   */
  tRegister(std::atomic<size_t>* external_size_variable = nullptr) :
    size(0),
    external_size_variable(external_size_variable)
  {
    chunks.fill(nullptr);
    chunks[0] = &first_chunk;
  }

  tRegister(const tRegister&) = delete; // make noncopyable
  tRegister& operator=(const tRegister&) = delete; // make noncopyable
  tRegister(tRegister&&) = default; // make nonmovable
  tRegister& operator=(tRegister &&) = default; // make nonmovable


  ~tRegister()
  {
    for (size_t i = 1; i < chunks.size(); i++)
    {
      if (chunks[i] == nullptr)
      {
        return;
      }
      delete chunks[i];
    }
  }

  /*!
   * Adds entry to register
   *
   * \param entry Entry to add
   * \return Index in register
   */
  size_t Add(const TEntry& entry)
  {
    rrlib::thread::tLock(*this);
    size_t size = Size();
    size_t chunk_index = size / Tchunk_size;
    if (chunk_index >= Tchunk_count)
    {
      throw std::length_error("Adding element exceeds register size (possibly increase register's Tchunk_count or Tchunk_size compile-time constants)");
    }
    size_t chunk_element_index = size % Tchunk_size;
    if (chunk_index && chunk_element_index == 0)
    {
      // Allocate new chunk
      assert(chunks[chunk_index] == nullptr);
      chunks[chunk_index] = new tChunk();
    }
    (*chunks[chunk_index])[chunk_element_index] = entry;

    size++;
    this->size = size;
    if (external_size_variable)
    {
      (*external_size_variable) = size;
    }
    for (auto & listener : listeners)
    {
      listener.second();
    }
    return size - 1;
  }

  /*!
   * Adds listener to register.
   *
   * \param callback Callback function that will be called whenever a new element is added.
   * \param address Address of listener. Optional: only required for identification when removing listener.
   */
  void AddListener(const std::function<void()>& callback, const void* address = nullptr) const
  {
    rrlib::thread::tLock(*this);
    listeners.emplace_back(address, callback);
  }

  /*!
   * \return Begin iterator for path elements
   */
  tConstIterator Begin() const
  {
    return tConstIterator(*this, 0);
  }

  /*!
   * Adds entry to register
   *
   * \param constructor_arguments Constructor arguments that are forwarded to TEntry constructor
   */
  template <typename... TArguments>
  void Emplace(TArguments && ... constructor_arguments)
  {
    rrlib::thread::tLock(*this);
    size_t size = Size();
    size_t chunk_index = size / Tchunk_size;
    if (chunk_index >= Tchunk_count)
    {
      throw std::length_error("Adding element exceeds register size (possibly increase register's Tchunk_count or Tchunk_size compile-time constants)");
    }
    size_t chunk_element_index = size % Tchunk_size;
    if (chunk_index && chunk_element_index == 0)
    {
      // Allocate new chunk
      assert(chunks[chunk_index] == nullptr);
      chunks[chunk_index] = new tChunk();
    }
    (*chunks[chunk_index])[chunk_element_index] = TEntry(constructor_arguments...);

    size++;
    this->size = size;
    if (external_size_variable)
    {
      (*external_size_variable) = size;
    }
    for (auto & listener : listeners)
    {
      listener.second();
    }
  }

  /*!
   * \return End iterator for path elements
   */
  tConstIterator End() const
  {
    return tConstIterator(*this, Size());
  }

  /*!
   * Remove listener from register.
   *
   * \param address Address of listener to remove
   * \return Whether listener was removed
   */
  bool RemoveListener(const void* address) const
  {
    rrlib::thread::tLock(*this);
    for (auto it = listeners.begin(); it < listeners.end(); ++it)
    {
      if (it->first == address)
      {
        listeners.erase(it);
        return true;
      }
    }
    return false;
  }

  /*!
   * \return Current number of elements in register
   */
  size_t Size() const
  {
    return size.load();
  }

  /*!
   * \return std::atomic containing current size
   */
  const std::atomic<size_t>& SizeAtomic() const
  {
    return size;
  }


  const TEntry& operator[](size_t index) const
  {
    size_t chunk_index = index / Tchunk_size;
    size_t chunk_element_index = index % Tchunk_size;
    return (*chunks[chunk_index])[chunk_element_index];
  }

  /*!
   * Iterator over elements
   */
  class tConstIterator
  {
  public:

    tConstIterator(const tRegister& register_instance, size_t element_index) :
      register_instance(register_instance),
      element_index(element_index)
    {
    }

    friend bool operator==(const tConstIterator& lhs, const tConstIterator& rhs)
    {
      return &lhs.register_instance == &rhs.register_instance && lhs.element_index == rhs.element_index;
    }
    friend bool operator!=(const tConstIterator& lhs, const tConstIterator& rhs)
    {
      return !(lhs == rhs);
    }
    friend bool operator<(const tConstIterator& lhs, const tConstIterator& rhs)
    {
      return lhs.element_index < rhs.element_index;
    }
    const TEntry& operator*() const
    {
      return register_instance[element_index];
    }
    inline const TEntry* operator->() const
    {
      return &(operator*());
    }
    inline tConstIterator& operator++()
    {
      element_index++;
      return *this;
    }
    inline tConstIterator operator ++ (int)
    {
      tConstIterator temp(*this);
      operator++();
      return temp;
    }

  private:

    const tRegister& register_instance;
    size_t element_index;
  };

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

  typedef std::array<TEntry, Tchunk_size> tChunk; //!< Type of single chunk

  tChunk first_chunk;                            //!< First chunk - allocated immediately
  std::array<tChunk*, Tchunk_count> chunks;       //!< All chunks
  std::atomic<size_t> size;                      //!< Internal size variable
  std::atomic<size_t>* external_size_variable;   //!< External size variable (optional - can be placed in header for efficient size lookup)
  mutable std::vector<std::pair<const void*, std::function<void()>>> listeners;   //!< Change Listeners (address and callback; address is only required for removal)
};


template <typename TEntry, size_t Tchunk_count, size_t Tchunk_size, typename TMutex>
typename tRegister<TEntry, Tchunk_count, Tchunk_size, TMutex>::tConstIterator begin(const rrlib::concurrent_containers::tRegister<TEntry, Tchunk_count, Tchunk_size, TMutex>& reg)
{
  return reg.Begin();
}
template <typename TEntry, size_t Tchunk_count, size_t Tchunk_size, typename TMutex>
typename tRegister<TEntry, Tchunk_count, Tchunk_size, TMutex>::tConstIterator end(const rrlib::concurrent_containers::tRegister<TEntry, Tchunk_count, Tchunk_size, TMutex>& reg)
{
  return reg.End();
}

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}


#endif
