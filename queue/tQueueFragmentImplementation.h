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
/*!\file    rrlib/concurrent_containers/queue/tQueueFragmentImplementation.h
 *
 * \author  Max Reichardt
 *
 * \date    2012-10-13
 *
 * \brief   Contains tQueueFragmentImplementation
 *
 * \b tQueueFragmentImplementation
 *
 * Implementation of queue fragment for different types.
 *
 */
//----------------------------------------------------------------------
#ifndef __rrlib__concurrent_containers__queue__tQueueFragmentImplementation_h__
#define __rrlib__concurrent_containers__queue__tQueueFragmentImplementation_h__

//----------------------------------------------------------------------
// External includes (system with <>, local with "")
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// Internal includes with ""
//----------------------------------------------------------------------
#include "rrlib/concurrent_containers/queue/tIntrusiveQueueFragment.h"

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
//! Queue fragment implementation
/*!
 * Implementation of queue fragment for different types.
 */
template <typename T>
class tQueueFragmentImplementation
{
  // this type T is not supported yet
};

template <typename T, typename D>
class tQueueFragmentImplementation<std::unique_ptr<T, D>> :
      public tIntrusiveQueueFragment<T, std::is_base_of<tQueueableMost, T>::value, std::is_base_of<tQueueableSingleThreaded, T>::value>
{
public:
  typedef tIntrusiveQueueFragment<T, std::is_base_of<tQueueableMost, T>::value, std::is_base_of<tQueueableSingleThreaded, T>::value> tBase;
  typedef std::unique_ptr<T, D> tPointer;

  tQueueFragmentImplementation() {}
  tQueueFragmentImplementation(tQueueFragmentImplementation && other) : tBase(std::forward<tBase>(other)) {}
  tQueueFragmentImplementation& operator=(tQueueFragmentImplementation && other)
  {
    tBase::operator=(std::forward<tBase>(other));
    return *this;
  }

  ~tQueueFragmentImplementation()
  {
    tBase::template DeleteObsoleteElements<T, D>();
    while (!this->Empty())
    {
      PopAny();
    }
  }

  tPointer PopAny()
  {
    return tPointer(tBase::PopAny());
  }

  tPointer PopBack()
  {
    return tPointer(tBase::PopBack());
  }

  tPointer PopFront()
  {
    return tPointer(tBase::PopFront());
  }
};


//----------------------------------------------------------------------
// End of namespace declaration
//----------------------------------------------------------------------
}
}
}


#endif
