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
/*!\file    rrlib/concurrent_containers/tSet.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-04
 *
 * \brief   Contains tSet
 *
 * \b tSet
 *
 * Set of elements.
 * Can be used with different storage policies that offer different levels of concurrency.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__tSet_h__
#define __rrlib__concurrent_containers__tSet_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------
#include "rrlib/logging/messages.h"

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

/*!
 * Determines whether an element can be added to a set multiple times.
 */
enum class tAllowDuplicates
{
  NO,   //!< Attempting to add an element that already is in the set, does not modify the set. Equality of two elements in checked via the '==' operator.
  YES,  //!< An element can be added multiple times.
  YES_OPTIMIZED  //!< Same as above with more efficient adding of elements at a slightly increased memory footprint (typically additional size_t variable that stores first free slot)
};

/*!
 * Provides the default "null element" for sets
 */
template <typename T>
struct NullElementDefault
{
  static constexpr T cNULL_ELEMENT = 0;
};


//----------------------------------------------------------------------
// Class declaration
//----------------------------------------------------------------------
//! Concurrent set
/*!
 * Set of elements.
 * Can be used with different storage policies that offer different levels of concurrency.
 * Iterating is always lock-free and can therefore be done with real-time threads
 * (the only exception is single-threaded storage policy guarded with locks).
 * Typically based on array lists, iterating is quick and memory consumption low.
 * Modifying is typically quite expensive, though (O(n)).
 *
 * \tparam T Type of list elements. T must be suitable for std::atomic<T> or a unique_ptr type.
 *           (Otherwise removing of elements concurrently to reading would cause issues)
 * \tparam ALLOW_DUPLICATES Can set contain an element multiple times? (see enum constants above)
 * \tparam TMutex Type of mutex to use for non-concurrent list operations (typically concurrent modifying calls).
 *                May be set to tNoMutex if concurrent calls to these operations cannot occur.
 *                Otherwise it should be set to tMutex.
 * \tparam TStoragePolicy Determines list implementation and which calls may be executed concurrently.
 * \tparam TNullElement Element that marks empty "slots" in backend. It may not be a added to set.
 *                      Type needs constant 'cNULL_ELEMENT' that can be casted to type T.
 * \tparam DEREFERENCING_ITERATOR If true, iterator returns dereferenced type T
 *                                (naturally, only works with types that can be dereferenced such as pointer)
 */
template < typename T, tAllowDuplicates ALLOW_DUPLICATES, typename TMutex, class TStoragePolicy,
         bool DEREFERENCING_ITERATOR = false, typename TNullElement = NullElementDefault<T >>
         class tSet : TStoragePolicy::template tInstance<T, ALLOW_DUPLICATES, TMutex, TNullElement, DEREFERENCING_ITERATOR>
         {

           typedef typename TStoragePolicy::template tInstance<T, ALLOW_DUPLICATES, TMutex, TNullElement, DEREFERENCING_ITERATOR> tStoragePolicy;

//----------------------------------------------------------------------
// Public methods and typedefs
//----------------------------------------------------------------------
           public:

           /*!
            * Iterator types to iterate over the list's elements.
            * Input iterators.
            */
           typedef typename tStoragePolicy::tConstIterator tConstIterator;

           /*!
            * Adds element to this set (unless element is already in the set and duplicates are not allowed)
            *
            * \param element Element to add
            */
           void Add(const T& element)
{
  if (element == static_cast<T>(TNullElement::cNULL_ELEMENT))
  {
    RRLIB_LOG_PRINT(ERROR, "The 'null element' may not be added to set. Ignoring. Please fix your code.");
    return;
  }
  tStoragePolicy::Add(element);
}

/*!
 * \return An iterator to iterate over this set's elements.
 * Initially points to the first element.
 *
 * Typically used in this way (set is a tSet reference):
 *
 *   for (auto it = set.Begin(); it != set.End(); ++it)
 *   {
 *       ...
 *   }
 */
tConstIterator Begin() const
{
  return tStoragePolicy::Begin();
}

/*!
 * Removes all elements from set
 */
void Clear()
{
  return tStoragePolicy::Clear();
}

/*!
 * \return True if set is empty
 */
bool Empty() const
{
  return tStoragePolicy::Empty();
}

/*!
 * \return An iterator to iterate over this set's elements pointing to the past-the-end element.
 */
tConstIterator End() const
{
  return tStoragePolicy::End();
}

/*!
 * Removes element at specified position from set.
 *
 * \param position Iterator pointing to element to be removed
 * \return Iterator pointing to the location of the element that followed erased element possibly list end)
 */
tConstIterator Remove(tConstIterator position)
{
  return tStoragePolicy::Remove(position);
}

/*!
 * Removes specified element from set.
 * If the set contains this element multiple times, it is removed multiple times (== operator is used to check equality)
 *
 * \param element Element to remove from set
 */
void Remove(const T& element)
{
  if (element == TNullElement::cNULL_ELEMENT)
  {
    // not in set by spec
    return;
  }
  tStoragePolicy::Remove(element);
}

//----------------------------------------------------------------------
// Private fields and methods
//----------------------------------------------------------------------
private:

         };

//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}

#include "rrlib/concurrent_containers/policies/set/storage/ArrayChunkBased.h"

#endif
